/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 Intel Corporation
 */

#ifndef _XMSS_ENGINE_H
#define _XMSS_ENGINE_H

/*
 * struct xmss_pub_key - Structure representing an XMSS public key.
 * @oid_len:	The length (in bytes) of the OID parameter.
 * @n:		The 'n' parameter of the public key; it's the length of both
 *		'root' and 'seed').
 * @oid:	The OID parameter of the public key.
 * @root:	The Root parameter of the public key.
 * @seed:	The Seed parameter of the public key.
 */
struct xmss_pub_key {
	int oid_len;
	int n;
	const void *oid;
	const void *root;
	const void *seed;
};

/**
 * xmss_engine_verify() - Verify a hash against an XMSS signature.
 * @dev:	The device to use.
 * @pubk:	The public key to use for signature verification.
 * @hash:	The hash to verify (as an array of bytes).
 * @hash_len:	The size of the hash (in bytes).
 * @sig:	The signature (as an array of bytes).
 * @sig_len:	The size of the signature (in bytes).
 *
 * Return:	0 if verified, negative value otherwise.
 */
int xmss_engine_verify(struct udevice *dev, const struct xmss_pub_key *pubk,
		       const uint8_t *hash, const size_t hash_len,
		       const uint8_t *sig, const size_t sig_len);

/**
 * struct xmss_engine_ops - Driver model for XMSS operations.
 */
struct xmss_engine_ops {
	/**
	 * Verify a hash against an XMSS signature.
	 * @dev:	The device to use.
	 * @pubk:	The public key to use for signature verification.
	 * @hash:	The hash to verify (as an array of bytes).
	 * @hash_len:	The size of the hash (in bytes).
	 * @sig:	The signature (as an array of bytes).
	 * @sig_len:	The size of the signature (in bytes).
	 *
	 * Return:	0 if verified, negative value otherwise.
	 */
	int (*verify)(struct udevice *dev, const struct xmss_pub_key *pubk,
		      const uint8_t *hash, const size_t hash_len,
		      const uint8_t *sig, const size_t sig_len);
};

#endif /* _XMSS_ENGINE_H */
