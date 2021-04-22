/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019, Intel Corporation
 */

#ifndef _XMSS_H
#define _XMSS_H

#include <errno.h>
#include <image.h>

#if IMAGE_ENABLE_SIGN && CONFIG_XMSS
/**
 * sign() - Calculate and return signature for given input image regions.
 * @info:	[in]  Specifies key and FIT information.
 * @region:	[in]  The array of image regions to process.
 * @region_cnt: [in]  The number of regions.
 * @sigp:	[out] Where to store the pointer to the buffer holding the
 *		      signature.
 * @sig_len:	[out] Where to store the length of the calculated hash.
 *
 * This computes input region signature according to the XMSS algorithm.
 * Resulting signature value is placed in an allocated buffer, the
 * pointer is returned as *sigp. The length of the calculated
 * signature is returned via the sig_len pointer argument. The caller
 * should free *sigp.
 *
 * @return: 0, on success, -ve on error
 */
int fit_xmss_sign(struct image_sign_info *info,
		  const struct image_region region[],
		  int region_cnt, uint8_t **sigp, uint *sig_len);

/**
 * add_verify_data() - Add verification information to FDT.
 * @info:	Specifies key and FIT information
 * @keydest:	Destination FDT blob for public key data
 *
 * Add public key information to the FDT node, suitable for verification at
 * run-time. For XMSS such information is the values of 'x' and 'y' and their
 * length in bits.
 *
 * Return: 0, on success, -ENOSPC if the keydest FDT blob ran out of space,
 *	   other -ve value on error
 */
int fit_xmss_add_verify_data(struct image_sign_info *info, void *keydest);
#else /* IMAGE_ENABLE_SIGN && CONFIG_XMSS */
static inline int fit_xmss_sign(struct image_sign_info *info,
				const struct image_region region[],
				int region_cnt, uint8_t **sigp, uint *sig_len)
{
	return -ENXIO;
}

static inline int fit_xmss_add_verify_data(struct image_sign_info *info,
					   void *keydest)
{
	return -ENXIO;
}
#endif /* IMAGE_ENABLE_SIGN && CONFIG_XMSS */

#if IMAGE_ENABLE_VERIFY && CONFIG_XMSS
/**
 * xmss_verify() - Verify a signature against some data
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
int fit_xmss_verify(struct image_sign_info *info,
		    const struct image_region region[], int region_cnt,
		    uint8_t *sig, uint sig_len);
#else /* IMAGE_ENABLE_VERIFY && CONFIG_XMSS */
static inline int fit_xmss_verify(struct image_sign_info *info,
				  const struct image_region region[],
				  int region_cnt, uint8_t *sig, uint sig_len)
{
	return -ENXIO;
}
#endif /* IMAGE_ENABLE_VERIFY && CONFIG_XMSS */

#endif /* _XMSS_H */
