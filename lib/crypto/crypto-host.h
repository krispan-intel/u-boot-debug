/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (R) 2019 Intel Corporation
 */

#ifndef _CRYPTO_HOST_H
#define _CRYPTO_HOST_H

#include <image.h>
#include <mkimage.h>
#include <openssl/evp.h>

struct crypto_host_algo_hlpr {
	int key_type;
	int engine_flag;
	int (*add_pubk_data_to_node)(EVP_PKEY *evp_pubk, void *fdt, int node);
};

int crypto_host_engine_init(const char *engine_id, ENGINE **pe,
			    int engine_flag);

void crypto_host_engine_deinit(ENGINE *e);

int crypto_host_get_pub_key(const char *keydir, const char *name,
			    ENGINE *engine, EVP_PKEY **evp_pubkp);

int crypto_host_sign(struct image_sign_info *info,
		     const struct image_region region[], int region_count,
		     uint8_t **sigp, uint *sig_len, int key_type,
		     int engine_flag);

int crypto_host_add_verify_data(struct image_sign_info *info, void *keydest,
				const struct crypto_host_algo_hlpr *algo);

int crypto_host_err(const char *fmt, ...);

#endif /* _CRYPTO_HOST_H */
