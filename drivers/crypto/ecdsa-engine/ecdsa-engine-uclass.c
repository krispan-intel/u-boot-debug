// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019, Intel Corporation
 */

#include <common.h>
#include <dm.h>
#include <u-boot/ecdsa-engine.h>

int ecdsa_engine_verify(struct udevice *dev, const struct ec_pub_key *pubk,
			const uint8_t *hash, const size_t hash_len,
			const uint8_t *sig, const size_t sig_len)
{
	const struct ecdsa_engine_ops *ops = device_get_ops(dev);

	if (!ops->verify) {
		debug("%s: ECDSA device has no verify function\n", __func__);
		return -ENOSYS;
	}

	return ops->verify(dev, pubk, hash, hash_len, sig, sig_len);
}

UCLASS_DRIVER(crypto_ecdsa) = {
	.id		= UCLASS_ECDSA_ENGINE,
	.name		= "ecdsa_engine",
};
