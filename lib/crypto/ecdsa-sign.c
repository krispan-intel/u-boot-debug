// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019, Intel Corporation
 * Copyright (c) 2013, Google Inc.
 */

#include "mkimage.h"
#include <stdio.h>
#include <string.h>
#include <image.h>
#include <time.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/engine.h>

#include "crypto-host.h"

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
#define HAVE_ERR_REMOVE_THREAD_STATE
#endif

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
#define ECDSA_ENGINE_FLAG	ENGINE_METHOD_EC
#else
#define ECDSA_ENGINE_FLAG	ENGINE_METHOD_ECDSA
#endif

/* Calculate the number of bytes needed to hold the specified number of bits. */
#define BITS_TO_BYTES(bits)	(((bits) + 7) / 8)

static int ecdsa_add_pubk_data_to_node(EVP_PKEY *evp_pubk, void *fdt, int node);

static const struct crypto_host_algo_hlpr ecdsa_algo = {
	.key_type = EVP_PKEY_EC,
	.engine_flag = ECDSA_ENGINE_FLAG,
	.add_pubk_data_to_node = ecdsa_add_pubk_data_to_node,
};

int ecdsa_sign(struct image_sign_info *info, const struct image_region region[],
	       int region_count, uint8_t **sigp, uint *sig_len)
{
	return crypto_host_sign(info, region, region_count, sigp, sig_len,
				EVP_PKEY_EC, ECDSA_ENGINE_FLAG);
}

/*
 * ecdsa_get_params(): - Get the important parameters of an ECDSA public key
 */
static int ecdsa_get_params(EC_KEY *key, BIGNUM **x, BIGNUM **y,
			    const char **curve_name)
{
	const EC_POINT *point;
	const EC_GROUP *group;
	BN_CTX *bn_ctx;
	int rc;

	if (!key || !x || !y || !curve_name)
		return -EINVAL;

	point = EC_KEY_get0_public_key(key);
	group = EC_KEY_get0_group(key);
	/* Get the NIST curve name. */
	*curve_name = EC_curve_nid2nist(EC_GROUP_get_curve_name(group));
	if (!*curve_name) {
		fprintf(stderr, "Failed to get curve name\n");
		return -ENOENT;
	}

	/* Allocate the context needed to manipulate Big Numbers. */
	bn_ctx = BN_CTX_new();
	if (!bn_ctx)
		return -ENOMEM;
	/* Allocate BIGNUMs for 'x' and 'y'. */
	*x = BN_new();
	*y = BN_new();
	if (!*x || !*y) {
		fprintf(stderr, "Out of memory (bignum)\n");
		rc = -ENOMEM;
		goto error;
	}
	/* Get 'x' and 'y'. */
	rc = EC_POINT_get_affine_coordinates_GFp(group, point, *x, *y, bn_ctx);
	if (!rc) {
		fprintf(stderr, "Failed to get public key point coordinates\n");
		rc = -ENOMEM;
		goto error;
	}

	BN_CTX_free(bn_ctx);
	return 0;
error:
	/* Note: BN_free(NULL) does nothing. */
	BN_free(*x);
	BN_free(*y);
	BN_CTX_free(bn_ctx);
	return rc;
}

static int fdt_add_bignum(void *blob, int noffset, const char *prop_name,
			  BIGNUM *num)
{
	const int size = BN_num_bytes(num);
	uint8_t *buf;
	int ret;

	buf = malloc(size);
	if (!buf) {
		fprintf(stderr, "Out of memory (%d bytes)\n", size);
		return -ENOMEM;
	}
	BN_bn2bin(num, buf);
	ret = fdt_setprop(blob, noffset, prop_name, buf, size);
	free(buf);

	return ret ? -FDT_ERR_NOSPACE : 0;
}

static int ecdsa_add_pubk_data_to_node(EVP_PKEY *evp_pubk, void *fdt, int node)
{
	BIGNUM *x, *y;
	EC_KEY *ec_pubk;
	const char *curve_name;
	int ret;

	/* Convert EVP key to a ECDSA_style key. */
	ec_pubk = EVP_PKEY_get1_EC_KEY(evp_pubk);
	if (!ec_pubk) {
		crypto_host_err("Couldn't convert public key to a ECDSA style key");
		return -EINVAL;
	}
	ret = ecdsa_get_params(ec_pubk, &x, &y, &curve_name);
	if (ret)
		goto err_get_params;
	/* Add public key info to key node. */
	ret = fdt_setprop_string(fdt, node, "ecdsa,curve", curve_name);
	if (ret)
		goto exit;
	ret = fdt_add_bignum(fdt, node, "ecdsa,x", x);
	if (ret)
		goto exit;
	ret = fdt_add_bignum(fdt, node, "ecdsa,y", y);
exit:
	BN_free(x);
	BN_free(y);
err_get_params:
	EC_KEY_free(ec_pubk);
	return ret;
}

int ecdsa_add_verify_data(struct image_sign_info *info, void *keydest)
{
	return crypto_host_add_verify_data(info, keydest, &ecdsa_algo);
}
