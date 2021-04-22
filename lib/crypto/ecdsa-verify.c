// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019-2020, Intel Corporation
 * Copyright (c) 2013, Google Inc.
 */

#ifndef USE_HOSTCC
#include <common.h>
#include <fdtdec.h>
#include <asm/types.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <asm/types.h>
#include <asm/unaligned.h>
#include <dm.h>
#else
#include "fdt_host.h"
#include "mkimage.h"
#include <fdt_support.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/err.h>
#endif
#include <u-boot/ecdsa.h>
#include <u-boot/ecdsa-engine.h>

#include "crypto-common.h"

#if USE_HOSTCC
/**
 * _ecdsa_verify_key() - Verify an ECDSA signature using OpenSSL.
 * @pubk:	The public key to use for signature verification.
 * @hash:	The hash to verify (as an array of bytes).
 * @hash_len:	The size of the hash (in bytes).
 * @sig:	The signature (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 *
 * Return:	0 if verified, negative value otherwise.
 */
static int _ecdsa_verify_key(const struct ec_pub_key *pubk,
			     const uint8_t *hash, const size_t hash_len,
			     const uint8_t *sig, const size_t sig_len)
{
	BIGNUM *x, *y;
	EC_KEY *pub_key = NULL;
	int nid, ret = -1;

	/* Check that curve name is valid. */
	nid = EC_curve_nist2nid(pubk->curve_name);
	if (nid == NID_undef) {
		fprintf(stderr, "Invalid curve name %s\n", pubk->curve_name);
		return -1;
	}

	x = BN_bin2bn(pubk->x, pubk->x_len, NULL);
	y = BN_bin2bn(pubk->y, pubk->y_len, NULL);

	/* Create empty key using the specified curve. */
	pub_key = EC_KEY_new_by_curve_name(nid);
	if (!x || !y || !pub_key) {
		fprintf(stderr, "Cannot allocate public key\n");
		goto exit;
	}
	/* Set public key. */
	if (!EC_KEY_set_public_key_affine_coordinates(pub_key, x, y)) {
		fprintf(stderr, "Cannot set public key\n");
		goto exit;
	}
	/* Verify signature (1 = OK, 0 = fail, -1 = error). */
	ret = ECDSA_verify(0, hash, hash_len, sig, sig_len, pub_key);
	if (ret < 0) {
		fprintf(stderr, "ECDSA_verify error: %s\n",
			ERR_error_string(ERR_get_error(), 0));
		goto exit;
	}
	ret = (ret == 1) ? 0 : -1;

exit:
	EC_KEY_free(pub_key);
	BN_free(x);
	BN_free(y);

	return ret;
}
#else /* USE_HOSTCC */
/**
 * _ecdsa_verify_key() - Verify an ECDSA signature using U-Boot ECDSA engine.
 * @pubk:	The public key to use for signature verification.
 * @hash:	The hash to verify (as an array of bytes).
 * @hash_len:	The size of the hash (in bytes).
 * @sig:	The signature (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 *
 * Return:	0 if verified, negative value otherwise.
 */
static int _ecdsa_verify_key(const struct ec_pub_key *pubk,
			     const uint8_t *hash, const size_t hash_len,
			     const uint8_t *sig, const size_t sig_len)
{
	struct udevice *ecdsa_dev;
	int ret;

	debug("%s()\n", __func__);
	ret = uclass_get_device(UCLASS_ECDSA_ENGINE, 0, &ecdsa_dev);
	if (ret) {
		log_err("ECDSA: Can't find any ECDSA engine\n");
		return -EINVAL;
	}
	ret = ecdsa_engine_verify(ecdsa_dev, pubk, hash, hash_len, sig,
				  sig_len);

	return ret;
}
#endif /* USE_HOSTCC */

/**
 * ecdsa_verify_key() - Verify an ECDSA signature.
 * @pubk:	The public key to use for signature verification.
 * @hash:	The hash to verify (as an array of bytes).
 * @hash_len:	The size of the hash (in bytes).
 * @sig:	The signature (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 *
 * Verify the ECDSA signature for the specified hash using the ECDSA Key
 * properties in pubk structure.
 *
 * Return:	0 if verified, -ve on error
 */
static int ecdsa_verify_key(struct ec_pub_key *pubk,
			    const uint8_t *hash, size_t hash_len,
			    const uint8_t *sig,  uint32_t sig_len)
{
	debug("%s()\n", __func__);
	if (!pubk || !sig || !hash)
		return -EIO;

	return _ecdsa_verify_key(pubk, hash, hash_len, sig, sig_len);
}

/**
 * ecdsa_verify_with_keynode() - Verify a signature using public key FDT node.
 * @info:	Information about the key and the FIT image.
 * @hash:	The hash to verify (as an array of bytes).
 * @hash_len:	The size of the hash (in bytes).
 * @sig:	The signature (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 * @node:	Node having the ECDSA Public Key properties.
 *
 * Parse sign-node and fill a pub_key structure with the properties of the key.
 * Verify the ECDSA signature of the specified hash using the properties
 * parsed.
 *
 * Return:	0 if verified, -ve on error
 */
static int ecdsa_verify_with_keynode(const struct image_sign_info *info,
				     const uint8_t *hash, size_t hash_len,
				     const uint8_t *sig, size_t sig_len,
				     int node)
{
	const void *blob = info->fdt_blob;
	struct ec_pub_key pubk;
	int len, ret = 0;

	if (node < 0) {
		debug("%s: Skipping invalid node\n", __func__);
		return -EBADF;
	}


	pubk.x = fdt_getprop(blob, node, "ecdsa,x", &pubk.x_len);
	pubk.y = fdt_getprop(blob, node, "ecdsa,y", &pubk.y_len);
	pubk.curve_name = fdt_getprop(blob, node, "ecdsa,curve", &len);
	/* Ensure that we have all the properties. */
	if (!pubk.x || !pubk.y || !pubk.curve_name) {
		debug("%s: Missing ECDSA key info\n", __func__);
		return -EFAULT;
	}
	/* Ensure that curve_name is a valid string. */
	if (len < 1 || pubk.curve_name[len - 1] != '\0') {
		debug("%s: ecdsa,curve is not a valid string\n", __func__);
		return -EFAULT;
	}

	ret = ecdsa_verify_key(&pubk, hash, hash_len, sig, sig_len);

	return ret;
}

/* See documentation in header file. */
int ecdsa_verify(struct image_sign_info *info,
		 const struct image_region region[], int region_count,
		 uint8_t *sig, uint sig_len)
{
	/*
	 * Verify that the digest used is SHA-384 (i.e., the only one we
	 * support with ECDSA).
	 */
	if (strcmp(info->checksum->name, "sha384")) {
		debug("%s: invalid checksum-algorithm %s for %s\n",
		      __func__, info->checksum->name, info->crypto->name);
		return -EINVAL;
	}
	return crypto_verify(info, region, region_count, sig, sig_len,
			     ecdsa_verify_with_keynode);
}
