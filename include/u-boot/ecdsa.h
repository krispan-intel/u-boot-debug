/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2019, Intel Corporation
 * Copyright (c) 2013, Google Inc.
 * (C) Copyright 2008 Semihalf
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef _ECDSA_H
#define _ECDSA_H

#include <errno.h>
#include <image.h>

/* The size of the ECDSA private key. */
#define ECDSA384_PRIV_KEY_BYTES	(384 / 8)

struct image_sign_info;

#if IMAGE_ENABLE_SIGN
/**
 * sign() - Calculate and return signature for given input image regions.
 * @info:	[in]  Specifies key and FIT information.
 * @region:	[in]  The array of image regions to process.
 * @region_cnt: [in]  The number of regions.
 * @sigp:	[out] Where to store the pointer to the buffer holding the
 *		      signature.
 * @sig_len:	[out] Where to store the length of the calculated hash.
 *
 * This computes input region signature according to the ECDSA algorithm.
 * Resulting signature value is placed in an allocated buffer, the
 * pointer is returned as *sigp. The length of the calculated
 * signature is returned via the sig_len pointer argument. The caller
 * should free *sigp.
 *
 * @return: 0, on success, -ve on error
 */
int ecdsa_sign(struct image_sign_info *info, const struct image_region region[],
	       int region_cnt, uint8_t **sigp, uint *sig_len);

/**
 * add_verify_data() - Add verification information to FDT.
 * @info:	Specifies key and FIT information
 * @keydest:	Destination FDT blob for public key data
 *
 * Add public key information to the FDT node, suitable for verification at
 * run-time. For ECDSA such information is the values of 'x' and 'y' and their
 * length in bits.
 *
 * Return: 0, on success, -ENOSPC if the keydest FDT blob ran out of space,
 *	   other -ve value on error
 */
int ecdsa_add_verify_data(struct image_sign_info *info, void *keydest);
#else
static inline int ecdsa_sign(struct image_sign_info *info,
			     const struct image_region region[], int region_cnt,
			     uint8_t **sigp, uint *sig_len)
{
	return -ENXIO;
}

static inline int ecdsa_add_verify_data(struct image_sign_info *info,
					void *keydest)
{
	return -ENXIO;
}
#endif

#if IMAGE_ENABLE_VERIFY
/**
 * ecdsa_verify() - Verify a signature against some data
 * @info:	Specifies key and FIT information.
 * @region:	The array of image regions to verify.
 * @region_cnt: The number of regions.
 * @sig:	The signature.
 * @sig_len:	The number of bytes in signature.
 *
 * Verify a ECDSA signature against an expected hash.
 *
 * @return 0 if verified, -ve on error.
 */
int ecdsa_verify(struct image_sign_info *info,
		 const struct image_region region[], int region_cnt,
		 uint8_t *sig, uint sig_len);
#else
static inline int ecdsa_verify(struct image_sign_info *info,
			       const struct image_region region[],
			       int region_cnt, uint8_t *sig, uint sig_len)
{
	return -ENXIO;
}
#endif

#endif
