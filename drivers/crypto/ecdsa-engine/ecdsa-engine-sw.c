// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019, Intel Corporation
 */

#include <common.h>
#include <dm.h>
#include <u-boot/ecdsa-engine.h>

static int sw_verify(struct udevice *dev, const struct ec_pub_key *pubk,
		     const uint8_t *hash, const size_t hash_len,
		     const uint8_t *sig, const size_t sig_len)
{
	log_err("Error: SW ECDSA engine not implemented yet\n");
	/* TODO: implement software ECDSA verification. */
	return -ENXIO;
}

static const struct ecdsa_engine_ops ops = {
	.verify	= sw_verify,
};

U_BOOT_DRIVER(ecdsa_engine_sw) = {
	.name	= "ecdsa_engine_sw",
	.id	= UCLASS_ECDSA_ENGINE,
	.ops	= &ops,
	.flags	= DM_FLAG_PRE_RELOC,
};

U_BOOT_DEVICE(ecdsa_engine_sw) = {
	.name = "ecdsa_engine_sw",
};
