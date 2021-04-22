/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019-2020 Intel Corporation
 */

#ifndef _ECDSA_ENGINE_H
#define _ECDSA_ENGINE_H

/*
 * struct ec_pub_key - Structure containing an elliptic curve public key.
 * @x:	   The X coordinate of the public key.
 * @y:	   The Y coordinate of the public key.
 * @x_len: The size (in bytes) of X.
 * @y_len: The size (in bytes) of Y.
 * @curve_name: The NIST name of the elliptic curve to be used.
 */
struct ec_pub_key {
	const void *x;
	const void *y;
	int x_len;
	int y_len;
	const char *curve_name;
};

/**
 * ecdsa_engine_verify() - Verify a hash against an ECDSA signature.
 * @dev:	The device to use.
 * @pubk:	The public key to use for signature verification.
 * @hash:	The hash to verify (as an array of bytes).
 * @hash_len:	The size of the hash (in bytes).
 * @sig:	The signature (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 *
 * Return:	0 if verified, negative value otherwise.
 */
int ecdsa_engine_verify(struct udevice *dev, const struct ec_pub_key *pubk,
			const uint8_t *hash, const size_t hash_len,
			const uint8_t *sig, const size_t sig_len);

/**
 * struct ecdsa_engine_ops - Driver model for ECDSA operations.
 */
struct ecdsa_engine_ops {
	/**
	 * Verify a hash against an ECDSA signature.
	 * @dev:	The device to use.
	 * @pubk:	The public key to use for signature verification.
	 * @hash:	The hash to verify (as an array of bytes).
	 * @hash_len:	The size of the hash (in bytes).
	 * @sig:	The signature (as an array of bytes).
	 * @sig_len:	The size of the signature (in bytes).
	 *
	 * Return:	0 if verified, negative value otherwise.
	 */
	int (*verify)(struct udevice *dev, const struct ec_pub_key *pubk,
		      const uint8_t *hash, const size_t hash_len,
		      const uint8_t *sig, const size_t sig_len);
};

#endif /* _ECDSA_ENGINE_H */
