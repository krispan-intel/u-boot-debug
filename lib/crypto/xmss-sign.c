// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019, Intel Corporation
 */

#include "mkimage.h"
#include <openssl/evp.h>
#include <openssl/engine.h>
#include <openssl/xmss.h>
#include <u-boot/xmss.h>

#include "crypto-host.h"

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
#define HAVE_ERR_REMOVE_THREAD_STATE
#endif

/* XMSS engines are not supported yet. */
#define XMSS_ENGINE_FLAG	ENGINE_METHOD_NONE

static int xmss_add_pubk_data_to_node(EVP_PKEY *evp_pubk, void *fdt, int node);

static const struct crypto_host_algo_hlpr xmss_algo = {
	.key_type = EVP_PKEY_XMSS,
	.engine_flag = XMSS_ENGINE_FLAG,
	.add_pubk_data_to_node = xmss_add_pubk_data_to_node,
};

int fit_xmss_sign(struct image_sign_info *info,
		  const struct image_region region[],
		  int region_count, uint8_t **sigp, uint *sig_len)
{
	return crypto_host_sign(info, region, region_count, sigp, sig_len,
			   EVP_PKEY_XMSS, XMSS_ENGINE_FLAG);
}

static int xmss_add_pubk_data_to_node(EVP_PKEY *evp_pubk, void *fdt, int node)
{
	uint8_t *oid, *root, *seed;
	int n, oid_len;
	XMSS_KEY *xmss_pubk;
	int ret;

	/*
	 * Convert EVP key to an XMSS-style key.
	 *
	 * TODO: change this to EVP_PKEY_get1_XMSS_KEY() once implemented by
	 * XMSS OpenSSL extension.
	 *
	 * Until then, do not explicitly free the returned XMSS_KEY, since
	 * EVP_PKEY_get0_*() API says: "the reference count of the returned key
	 * is not incremented and so must not be freed up after use".
	 */
	xmss_pubk = EVP_PKEY_get0_XMSS_KEY(evp_pubk);
	if (!xmss_pubk) {
		crypto_host_err("Couldn't convert public key to an XMSS style key");
		return -EINVAL;
	}

	/* Get XMSS Public key paramenters. */
	oid  = XMSS_get_publickey_oid(xmss_pubk);
	oid_len = XMSS_get_oid_len();
	root = XMSS_get_publickey_root(xmss_pubk);
	seed = XMSS_get_publickey_seed(xmss_pubk);
	/* n is the length of root and seed. */
	n = XMSS_get_param_n(xmss_pubk);

	/* Add public key info to key node. */
	ret = fdt_setprop(fdt, node, "xmss,oid", oid, oid_len);
	if (ret)
		goto exit;
	ret = fdt_setprop_u32(fdt, node, "xmss,oid-len", oid_len);
	if (ret)
		goto exit;
	ret = fdt_setprop(fdt, node, "xmss,root", root, n);
	if (ret)
		goto exit;
	ret = fdt_setprop(fdt, node, "xmss,seed", seed, n);
	if (ret)
		goto exit;
	ret = fdt_setprop_u32(fdt, node, "xmss,n", n);
	if (ret)
		goto exit;
exit:
	return ret;
}

int fit_xmss_add_verify_data(struct image_sign_info *info, void *keydest)
{
	return crypto_host_add_verify_data(info, keydest, &xmss_algo);
}
