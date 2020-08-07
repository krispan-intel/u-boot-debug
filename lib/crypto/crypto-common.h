/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (R) 2019 Intel Corporation
 */

#ifndef _CRYPTO_COMMON_H
#define _CRYPTO_COMMON_H

#include <image.h>

/**
 * verify_fn_t() - Verify a signature using public key FDT node.
 * @info:	Information about the key and the FIT image.
 * @hash:	The hash to verify (as an array of bytes).
 * @hash_len:	The size of the hash (in bytes).
 * @sig:	The signature (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 * @node:	Node having the algorithm-specific public key properties.
 *
 * Parse sign-node and fill a pub_key structure with the properties of the key.
 * Verify the signature of the specified hash using the properties parsed.
 *
 * Return:	0 if verified, -ve on error
 */
typedef int (verify_fn_t)(const struct image_sign_info *info,
			  const uint8_t *hash, size_t hash_len,
			  const uint8_t *sig, size_t sig_len,
			  int node);

/**
 * crypto_verify() - Verify image signature using the specific verify function.
 * @info:	Specifies key and FIT information.
 * @region:	The array of image regions to verify.
 * @region_cnt: The number of regions.
 * @sig:	The signature.
 * @sig_len:	The number of bytes in signature.
 * @verify_fn:  The verify function to be used to verify the signature.
 *
 * @return 0 if verified, -ve on error.
 */
int crypto_verify(struct image_sign_info *info,
		  const struct image_region region[], int region_count,
		  uint8_t *sig, uint sig_len,
		  verify_fn_t verify_fn);

#endif /* _CRYPTO_COMMON_H */
