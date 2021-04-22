// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019-2020, Intel Corporation
 */

#include <common.h>
#include <dm.h>
#include <asm/arch/sip_svc.h>
#include <u-boot/xmss.h>
#include <u-boot/xmss-engine.h>

/* Macros matching OID_LEN and PARAM_N XMSS macros in ATF. */
#define XMSS_OID_LEN		4
#define XMSS_PARAM_N		32
/* ATF is configured to use H = 10. */
#define XMSS_PARAM_H		10
#define XMSS_PARAM_L1		64
#define XMSS_PARAM_L2		3
#define XMSS_PARAM_L		(XMSS_PARAM_L1 + XMSS_PARAM_L2)
#define XMSS_SIG_IDX_LEN	4
#define XMSS_SIGN_LEN		(XMSS_SIG_IDX_LEN + XMSS_PARAM_N + \
				 (XMSS_PARAM_L * XMSS_PARAM_N) +   \
				 (XMSS_PARAM_H * XMSS_PARAM_N))

/*
 * Structure matching XMSSPubKey type defined in ATF and expected by the SiP
 * service.
 */
struct __packed sip_xmss_pubk {
	uint8_t oid[XMSS_OID_LEN];
	uint8_t root[XMSS_PARAM_N];
	uint8_t seed[XMSS_PARAM_N];
};

/*
 * Structure for holding XMSS verify parameters in shared memory. Parameters
 * must be stored in shared memory so that secure world can access it.
 */
struct xmss_shmem {
	uint8_t dgst[SHA256_SIZE];
	uint8_t sign[XMSS_SIGN_LEN];
	struct sip_xmss_pubk pubk;
};

/* The platdata for this driver. */
struct thunderbay_xmss_engine_plat {
	struct xmss_shmem *shmem;
};

/* The verify() exported by this driver. */
static int verify(struct udevice *dev, const struct xmss_pub_key *pubk,
		  const uint8_t *dgst, const size_t dgst_len,
		  const uint8_t *sign, const size_t sign_len)
{
	const struct thunderbay_xmss_engine_plat *plat = dev_get_plat(dev);
	struct xmss_shmem *shmem = plat->shmem;
	int rc;

	if (dgst_len != SHA256_SIZE) {
		debug("Error: ThunderBay XMSS: digest must be 256-bit long.\n");
		return -EINVAL;
	}
	if (sign_len != XMSS_SIGN_LEN) {
		debug("Error: ThunderBay XMSS: invalid sign length.\n");
		return -EINVAL;
	}
	if (pubk->oid_len != XMSS_OID_LEN) {
		debug("Error: ThunderBay XMSS: invalid OID length in key.\n");
		return -EINVAL;
	}
	if (pubk->n != XMSS_PARAM_N) {
		debug("Error: ThunderBay XMSS: invalid N parameter in key.\n");
		return -EINVAL;
	}
	/* Copy dgst and signature into shared memory. */
	memcpy(shmem->dgst, dgst, dgst_len);
	memcpy(shmem->sign, sign, sign_len);
	/* Copy public key parameters into shared memory. */
	memcpy(shmem->pubk.oid, pubk->oid, pubk->oid_len);
	memcpy(shmem->pubk.root, pubk->root, pubk->n);
	memcpy(shmem->pubk.seed, pubk->seed, pubk->n);

	/* Check digest signature using SiP services. */
	rc = sip_svc_xmss_verify(shmem->dgst, dgst_len,
				 shmem->sign, sign_len,
				 (uint8_t *)&shmem->pubk, sizeof(shmem->pubk));

	return rc;
}

/* Parse device tree to find location of shared memory to use. */
static int thunderbay_xmss_engine_ofdata_to_platdata(struct udevice *dev)
{
	struct thunderbay_xmss_engine_plat *plat = dev_get_plat(dev);
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
	    shmem_size < sizeof(struct xmss_shmem)) {
		debug("Invalid values for device shared memory.\n");
		return -EINVAL;
	}
	/* Store shared memory information in device platdata. */
	debug("shmem_addr = %llx, shmem_size = %llx\n", shmem_addr, shmem_size);
	plat->shmem = (void *)shmem_addr;

	return 0;
}

static const struct xmss_engine_ops ops = {
	.verify	= verify,
};

static const struct udevice_id thunderbay_xmss_engine_ids[] = {
	{.compatible = "intel,thunderbay-xmss" },
	{}
};

U_BOOT_DRIVER(thunderbay_xmss_engine) = {
	.name			  = "thunderbay_xmss_engine",
	.id			  = UCLASS_XMSS_ENGINE,
	.of_match		  = thunderbay_xmss_engine_ids,
	.of_to_plat		  = thunderbay_xmss_engine_ofdata_to_platdata,
	.ops			  = &ops,
	.plat_auto		  = sizeof(struct thunderbay_xmss_engine_plat),
};
