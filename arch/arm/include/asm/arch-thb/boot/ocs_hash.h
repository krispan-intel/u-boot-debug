/*
 * Copyright (c) 2019-2020 Intel Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 * Original ATF 'ocs_hash.h' file, after removing unneeded information.
 */

#ifndef __OCS_HASH_H__
#define __OCS_HASH_H__

/* SHA block sizes in bytes */
#define SHA1_BLOCK_SIZE (64)
#define SHA2_SHA224_BLOCK_SIZE (SHA1_BLOCK_SIZE)
#define SHA2_SHA256_BLOCK_SIZE (SHA1_BLOCK_SIZE)

/* SHA output digest sizes in bytes */
#define SHA1_SIZE (20)   /* 5 chains */
#define SHA224_SIZE (28) /* 7 chains */
#define SHA256_SIZE (32) /* 8 chains */
#define SHA384_SIZE (48) /* 12 chains. */
#define SHA512_SIZE (64) /* 16 chains. */
#define MD5_SIZE (16)    /* 4 chains */
#define SHA_MAX_SIZE SHA512_SIZE
#define HMAC_KEY_SIZE (SHA1_BLOCK_SIZE)
#define AES_256_KEY_BYTE_SIZE (32)
#define AES_256_KEY_U32_SIZE (AES_256_KEY_BYTE_SIZE >> 2)
#define AES_IV_BYTE_SIZE (16)
#define AES_IV_U32_SIZE (AES_IV_BYTE_SIZE >> 2)


/**
 * Hash type to specify the Hash Algorithm to be used by generic hash routines.
 */
typedef enum {
	OCS_HASH_SHA256 = 0, /**< Perform SHA256 Algorithm. */
	OCS_HASH_SHA384 = 1, /**< Perform SHA384 Algorithm. */
	OCS_HASH_SHA512 = 2, /**< Perform SHA512 Algorithm. */
	OCS_HASH_LIMIT = OCS_HASH_SHA512,
} ocs_hash_alg_t;

#endif /* __OCS_HASH_H__ */
