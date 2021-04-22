// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019-2020, Intel Corporation
 */

#include <common.h>
#include <u-boot/sha512.h>
#include <dm.h>
#include <asm/arch/sip_svc.h>
#include <u-boot/ecdsa-engine.h>

/* The size (in bytes) of each point coordinate. */
#define ECDSA384_COORD_SIZE	48
/* The size of an ASN.1 SEQUENCE header (0x30 <len>) when length <= 127. */
#define ASN1_SEQ_HDR	2
/* The size of an ASN.1 INTEGER header (0x02 <len>) when length <= 127. */
#define ASN1_INT_HDR	2
/* The minimum size of an ASN.1 INTEGER (i.e., the size of 0). */
#define ASN1_INT_MIN	(ASN1_INT_HDR + 1)
/*
 * Maximum size of an ECDSA-384 signature encoded with ASN.1, i.e., a sequence
 * of two integers (point coordinates) of maximum 384 bit (48 bytes).
 * Note: the actual encoded integer size can be up to 48 + 1 bytes, since a
 * leading 0 is added if the integer is negative (i.e., when its most
 * significant bit is 1).
 */
#define SIG_MAX_LEN	(ASN1_SEQ_HDR +					       \
			 (ASN1_INT_HDR + ECDSA384_COORD_SIZE + 1) * 2)

/*
 * Structure for holding ECDSA verify parameters in shared memory. Parameters
 * must be stored in shared memory so that secure world can access it.
 */
struct ecdsa_shmem {
	uint8_t x[ECDSA384_COORD_SIZE];
	uint8_t y[ECDSA384_COORD_SIZE];
	uint8_t r[ECDSA384_COORD_SIZE];
	uint8_t s[ECDSA384_COORD_SIZE];
	uint8_t hash[ECDSA384_COORD_SIZE];
};

/* The platdata for this driver. */
struct thunderbay_ecdsa_engine_plat {
	struct ecdsa_shmem *shmem;
};

/* Parse an ASN.1 ECDSA-384 signature and extract r and s. */
static int parse_asn1_sig_r_s(const uint8_t *sig, const size_t sig_len,
			      uint8_t *r, uint8_t *s)
{
	int n, r_start, s_start, r_len, s_len;
	int i = 0;
	bool ok = 1;

	if (unlikely(sig_len > SIG_MAX_LEN))
		return -1;
	/*
	 * Prevent out of bounds access while we parse the ASN.1 sequence and
	 * first integer headers.
	 */
	if (unlikely(sig_len < ASN1_SEQ_HDR + ASN1_INT_HDR))
		return -1;
	/* First byte must be an ASN.1 sequence tag (i.e, 0x30). */
	ok &= (sig[i++] == 0x30);
	/* Second byte is the length of the sequence. */
	ok &= (sig[i++] == (sig_len - ASN1_SEQ_HDR));
	/* First element of the sequence must be an integer (tag 0x02) */
	ok &= (sig[i++] == 0x02);
	/* Next byte is the length of the integer. */
	n = sig[i++];
	/* Prevent out of bounds access while processing the first integer. */
	if (unlikely(n + i) > sig_len)
		return -1;
	/*
	 * If the integer is positive but the high order bit is set to 1, a
	 * leading 0x00 is added to the content to indicate that the number is
	 * not negative. Check if that's the case (but only if n > 1).
	 */
	if (n > 1 && sig[i] == 0 && (sig[i + 1] & BIT(7))) {
		n--;
		i++;
	}
	/* We found where our first integer starts. */
	r_start = i;
	r_len = n;
	i += n;
	/* Parse second integer in a similar way. */
	if (unlikely((i + ASN1_INT_HDR) > sig_len))
		return -1;
	ok &= (sig[i++] == 0x02);
	n = sig[i++];
	if (unlikely(n + i) > sig_len)
		return -1;
	if (n > 1 && sig[i] == 0 && (sig[i + 1] & BIT(7))) {
		n--;
		i++;
	}
	s_start = i;
	s_len = n;
	i += n;
	/* Check that we reached the end of the signature. */
	ok &= (i == sig_len);
	if (unlikely(!ok))
		return -1;
	/* Now ensure that both 'r' and 's' are not bigger than expected. */
	if (unlikely(r_len > ECDSA384_COORD_SIZE)) {
		debug("'r' is too big: %d\n", r_len);
		return -1;
	}
	if (unlikely(s_len > ECDSA384_COORD_SIZE)) {
		debug("'s' is too big: %d\n", s_len);
		return -1;
	}
	/*
	 * If everything was fine so far, copy 'r' and 's' into the output
	 * buffers.
	 *
	 * Note: the encoded 'r' and 's' can be shorter than 48 bytes, that
	 * happens when the most significant bytes are 0. Therefore we need to
	 * manually set the initial bytes of our arrays to '0'.
	 */
	memset(r, 0, ECDSA384_COORD_SIZE - r_len);
	r += ECDSA384_COORD_SIZE - r_len;
	memcpy(r, &sig[r_start], r_len);
	memset(s, 0, ECDSA384_COORD_SIZE - s_len);
	s += ECDSA384_COORD_SIZE - s_len;
	memcpy(s, &sig[s_start], s_len);

	return 0;
}

/* The verify() exported by this driver. */
static int verify(struct udevice *dev, const struct ec_pub_key *pubk,
		  const uint8_t *hash, const size_t hash_len,
		  const uint8_t *sig, const size_t sig_len)
{
	const struct thunderbay_ecdsa_engine_plat *plat = dev_get_plat(dev);
	struct ecdsa_shmem *shmem = plat->shmem;
	int padding, rc;

	if (unlikely(hash_len != SHA384_SUM_LEN)) {
		debug("Error: Thunder Bay ECDSA: digest must be 384-bit long\n");
		return -EINVAL;
	}
	if (unlikely(strncmp("P-384", pubk->curve_name, strlen("P-384")))) {
		debug("Error: Thunder Bay ECDSA: invalid curve name\n");
		return -EINVAL;
	}
	if (unlikely(pubk->x_len > ECDSA384_COORD_SIZE)) {
		debug("Error: Thunder Bay ECDSA: pub key X coordinate too big\n");
		return -EINVAL;
	}
	if (unlikely(pubk->y_len > ECDSA384_COORD_SIZE)) {
		debug("Error: Thunder Bay ECDSA: pub key Y coordinate too big\n");
		return -EINVAL;
	}
	/*
	 * Copy public key coordinates into shared memory (padding them with
	 * leading zeros if needed).
	 */
	padding = ECDSA384_COORD_SIZE - pubk->x_len;
	memset(shmem->x, 0, padding);
	memcpy(shmem->x + padding, pubk->x, pubk->x_len);
	padding = ECDSA384_COORD_SIZE - pubk->y_len;
	memset(shmem->y, 0, padding);
	memcpy(shmem->y + padding, pubk->y, pubk->y_len);
	/* Copy hash into shared memory. */
	memcpy(shmem->hash, hash, hash_len);
	/* Parse signature and store r and s in shared memory. */
	rc = parse_asn1_sig_r_s(sig, sig_len, shmem->r, shmem->s);
	if (unlikely(rc)) {
		debug("Error: Thunder Bay ECDSA: cannot parse signature\n");
		return -EINVAL;
	}
	/* Check digest signature using SiP services. */
	rc = sip_svc_ecdsa_p384_verify(shmem->hash, hash_len, shmem->x,
				       shmem->y, shmem->r, shmem->s);
	if (unlikely(rc))
		debug("Error: Thunder Bay ECDSA: SiP service returned %d\n", rc);

	return rc;
}

/* Parse device tree to find location of shared memory to use. */
static int thunderbay_ecdsa_engine_ofdata_to_platdata(struct udevice *dev)
{
	struct thunderbay_ecdsa_engine_plat *plat = dev_get_plat(dev);
	int node = dev_of_offset(dev);
	int parent = dev_of_offset(dev->parent);
	int offset;
	fdt_addr_t shmem_addr;
	fdt_size_t shmem_size;

	/* Find device-tree node describing the shared memory to use. */
	offset = fdtdec_lookup_phandle(gd->fdt_blob, node, "shmem");
	if (offset < 0) {
		debug("No shared memory defined for this device: %d\n", offset);
		return -ENXIO;
	}
	/* Get the address and the size of the shared memory. */
	shmem_addr = fdtdec_get_addr_size_auto_parent(gd->fdt_blob, parent,
						      offset, "reg", 0,
						      &shmem_size, true);
	if (shmem_addr == FDT_ADDR_T_NONE ||
	    shmem_size < sizeof(struct ecdsa_shmem)) {
		debug("Invalid values for device shared memory.\n");
		return -EINVAL;
	}
	/* Store shared memory information in device platdata. */
	debug("shmem_addr = %llx, shmem_size = %llx\n", shmem_addr, shmem_size);
	plat->shmem = (void *)shmem_addr;

	return 0;
}

static const struct ecdsa_engine_ops ops = {
	.verify	= verify,
};

static const struct udevice_id thunderbay_ecdsa_engine_ids[] = {
	{.compatible = "intel,thunderbay-ecdsa" },
	{}
};

U_BOOT_DRIVER(thunderbay_ecdsa_engine) = {
	.name			  = "thunderbay_ecdsa_engine",
	.id			  = UCLASS_ECDSA_ENGINE,
	.of_match		  = thunderbay_ecdsa_engine_ids,
	.of_to_plat		  = thunderbay_ecdsa_engine_ofdata_to_platdata,
	.ops			  = &ops,
	.plat_auto		  = sizeof(struct thunderbay_ecdsa_engine_plat),
};
