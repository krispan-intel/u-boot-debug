// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019, Intel Corporation
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
#include "crypto-common.h"
#else
#include "fdt_host.h"
#include "mkimage.h"
#include <fdt_support.h>
#include <openssl/xmss.h>
#endif
#include <u-boot/xmss.h>
#include <u-boot/xmss-engine.h>

#include "crypto-common.h"

/* Parse the XMSS public key from the FDT Key node. */
static int parse_pub_key(struct xmss_pub_key *pubk, const void *blob, int node)
{
	int len;

	if (node < 0) {
		debug("%s: Skipping invalid node", __func__);
		return -EBADF;
	}
	/* Get 'oid-len' and 'n' parameters; they cannot be 0. */
	pubk->oid_len = fdtdec_get_int(blob, node, "xmss,oid-len", 0);
	pubk->n = fdtdec_get_int(blob, node, "xmss,n", 0);
	if (pubk->oid_len == 0 || pubk->n == 0)
		return -EINVAL;
	/* Get OID parameter; its length must be 'oid-len'. */
	pubk->oid = fdt_getprop(blob, node, "xmss,oid", &len);
	if (len != pubk->oid_len)
		return -EINVAL;
	/* Get Root and Seed parameters; their length must be 'n'. */
	pubk->root = fdt_getprop(blob, node, "xmss,root", &len);
	if (len != pubk->n)
		return -EINVAL;
	pubk->seed = fdt_getprop(blob, node, "xmss,seed", &len);
	if (len != pubk->n)
		return -EINVAL;
	return 0;
}

#if USE_HOSTCC
/**
 * host_xmss_verify() - Verify an XMSS signature using OpenSSL.
 * @info:	Information about the key and the FIT image.
 * @region:	The array of image regions to verify.
 * @region_cnt: The number of regions.
 * @sig:	The signature to verify (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 * @node:	The device-tree node containing the public key to use for the
 *		authentication.
 *
 * Return:	0 if verified, negative value otherwise.
 */
static int host_xmss_verify(const struct image_sign_info *info,
			    const struct image_region *region, int region_cnt,
			    const uint8_t *sig, const size_t sig_len, int node)
{
	struct xmss_pub_key pubk;
	XMSS_KEY *xmss_key;
	XMSS_PARAMS *xmss_params;
	EVP_PKEY *evp_pkey = NULL;
	EVP_MD_CTX *mdctx = NULL;
	int rc, i;

	rc = parse_pub_key(&pubk, info->fdt_blob, node);
	if (rc) {
		fprintf(stderr, "Failed to parse public key in device tree\n");
		return rc;
	}
	/* Allocate a new OpenSSL XMSS key. */
	xmss_key = XMSS_KEY_new();
	if (!xmss_key) {
		fprintf(stderr, "Cannot allocate XMSS public key\n");
		return -ENOMEM;
	}
	/*
	 * 'oid-len' is fixed in current XMSS OpenSSL code; check that the
	 * expected value matches ours.
	 */
	if (XMSS_get_oid_len() != pubk.oid_len) {
		rc = -EINVAL;
		goto free_xmss;
	}
	/* Set internal key parameters based on OID; these include 'n'. */
	xmss_params = XMSS_KEY_get_params(xmss_key);
	setParamsGivenOID(xmss_params, (uint8_t *)pubk.oid);
	/* Check that our 'n' matches the expected one. */
	if (XMSS_get_param_n(xmss_key) != pubk.n) {
		rc = -EINVAL;
		goto free_xmss;
	}
	/* Set OID, Root and Seed. */
	XMSS_set_publickey_oid(xmss_key, (uint8_t *)pubk.oid, pubk.oid_len);
	XMSS_set_publickey_root(xmss_key, (uint8_t *)pubk.root, pubk.n);
	XMSS_set_publickey_seed(xmss_key, (uint8_t *)pubk.seed, pubk.n);
	/*
	 * Create an EVP_PKEY to encapsulate our XMSS public key, so that we
	 * can use EVP_DigestVerify*() functions.
	 */
	evp_pkey = EVP_PKEY_new();
	if (!evp_pkey) {
		rc = -ENOMEM;
		goto free_xmss;
	}
	rc = EVP_PKEY_assign_XMSS_KEY(evp_pkey, xmss_key);
	if (rc != 1) {
		rc = -EINVAL;
		goto free_xmss;
	}
	/*
	 * Authenticate image regions using EVP_DigestVerify*() functions.
	 * These functions internally compute the hash (SHA-256) of the regions
	 * and then verify its signature using the specified public key. The
	 * progressive hash computation requires the use of a context (MD_CTX).
	 */
	mdctx = EVP_MD_CTX_create();
	if (!mdctx) {
		rc = -ENOMEM;
		goto free_evp;
	}
	rc = EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, evp_pkey);
	if (rc == 0) {
		rc = -EACCES;
		goto free_evp;
	}
	for (i = 0; i < region_cnt; i++) {
		rc = EVP_DigestVerifyUpdate(mdctx, region[i].data,
					    region[i].size);
		if (rc == 0) {
			rc = -EACCES;
			goto free_evp;
		}
	}
	/* Verify signature (1 = OK, 0 = fail, -1 = error). */
	rc = EVP_DigestVerifyFinal(mdctx, sig, sig_len);
	rc = (rc == 1) ? 0 : -EACCES;
	goto free_evp;

free_xmss:
	/*
	 * We should call XMSS_KEY_free only if EVP_PKEY_assign_XMSS_KEY() was
	 * not executed successfully.
	 */
	XMSS_KEY_free(xmss_key);
free_evp:
	/* Note: it's safe to free/destroy NULL pointers. */
	EVP_PKEY_free(evp_pkey);
	EVP_MD_CTX_destroy(mdctx);

	return rc;
}

/**
 * host_fit_xmss_verify() - Verify a signature via OpenSSL using key in 'info'.
 * @info:	Specifies key and FIT information.
 * @region:	The array of image regions to verify.
 * @region_cnt: The number of regions.
 * @sig:	The signature.
 * @sig_len:	The number of bytes in signature.
 *
 * Verify a XMSS signature against an expected hash.
 *
 * @return 0 if verified, -ve on error.
 */
static int host_fit_xmss_verify(struct image_sign_info *info,
				const struct image_region region[],
				int region_cnt,
				uint8_t *sig, uint sig_len)
{
	const void *blob = info->fdt_blob;
	/* Reserve memory for checksum-length */
	int ndepth, noffset;
	int sig_node, node;
	char name[100];
	int ret;

	/* Get signature FDT node. */
	sig_node = fdt_subnode_offset(blob, 0, FIT_SIG_NODENAME);
	if (sig_node < 0) {
		debug("%s: No signature node found\n", __func__);
		return -ENOENT;
	}

	/*
	 * Do not calculated the checksum, as it will be computed via OpenSSL
	 * in host_xmss_verify().
	 */

	/* See if we must use a particular key */
	if (info->required_keynode != -1) {
		ret = host_xmss_verify(info, region, region_cnt, sig, sig_len,
				       info->required_keynode);
		return ret;
	}

	/* Look for a key that matches our hint */
	snprintf(name, sizeof(name), "key-%s", info->keyname);
	node = fdt_subnode_offset(blob, sig_node, name);
	if (node >= 0) {
		ret = host_xmss_verify(info, region, region_cnt, sig, sig_len,
				       node);
		if (!ret)
			return 0;
	}

	/* No luck, so try each of the keys in turn */
	for (ndepth = 0, noffset = fdt_next_node(info->fit, sig_node, &ndepth);
			(noffset >= 0) && (ndepth > 0);
			noffset = fdt_next_node(info->fit, noffset, &ndepth)) {
		if (ndepth == 1 && noffset != node) {
			ret = host_xmss_verify(info, region, region_cnt,
					       sig, sig_len, noffset);
			if (!ret)
				return 0;
		}
	}

	return -EPERM;
}

#else /* USE_HOSTCC */

/**
 * uboot_xmss_verify() - Verify an XMSS signature in U-Boot.
 * @info:	Information about the key and the FIT image.
 * @hash:	The hash to verify (as an array of bytes).
 * @hash_len:	The size of the hash (in bytes).
 * @sig:	The signature to verify (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 * @node:	FDT node containing the XMSS Public Key properties.
 *
 * Return:	0 if verified, negative value otherwise.
 */
static int uboot_xmss_verify(const struct image_sign_info *info,
			     const uint8_t *hash, size_t hash_len,
			     const uint8_t *sig, size_t sig_len, int node)
{
	struct xmss_pub_key pubk;
	struct udevice *xmss_dev;
	int rc;

	rc = parse_pub_key(&pubk, info->fdt_blob, node);
	if (rc) {
		log_err("Failed to parse public key in device tree\n");
		return rc;
	}

	/* Call the XMSS engineto verify the signature of the hash. */
	rc = uclass_get_device(UCLASS_XMSS_ENGINE, 0, &xmss_dev);
	if (rc) {
		log_err("XMSS: Can't find any XMSS engine\n");
		return -EINVAL;
	}
	rc = xmss_engine_verify(xmss_dev, &pubk, hash, hash_len, sig,
				sig_len);

	return rc;
}
#endif /* USE_HOSTCC */

/* See documentation in header file. */
int fit_xmss_verify(struct image_sign_info *info,
		    const struct image_region region[], int region_cnt,
		    uint8_t *sig, uint sig_len)
{
	/*
	 * Verify that the digest used is SHA-256 (i.e., the only one we
	 * support with XMSS).
	 */
	if (strcmp(info->checksum->name, "sha256")) {
		debug("%s: invalid checksum-algorithm %s for %s\n",
		      __func__, info->checksum->name, info->crypto->name);
		return -EINVAL;
	}
#if USE_HOSTCC
	/*
	 * We cannot use crypto_verify() in the host tools because the current
	 * OpenSSL XMSS API does not provide a function equivalent to
	 * XMSS_verify(), i.e., a function to verify a hash which has been
	 * already computed.
	 */
	return host_fit_xmss_verify(info, region, region_cnt, sig, sig_len);
#else
	/*
	 * Use common logic in crypto_verify() and customize it by passing
	 * uboot_xmss_verify() as callback.
	 */
	return crypto_verify(info, region, region_cnt, sig, sig_len,
			     uboot_xmss_verify);
#endif
}
