// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019, Intel Corporation
 */

#include <common.h>
#include <dm.h>
#include <u-boot/xmss-engine.h>

static int sw_verify(struct udevice *dev, const struct xmss_pub_key *pubk,
		     const uint8_t *hash, const size_t hash_len,
		     const uint8_t *sig, const size_t sig_len)
{
	log_err("Error: SW XMSS engine not implemented yet\n");
	/* TODO: implement software XMSS verification. */
	return -ENXIO;
}

static const struct xmss_engine_ops ops = {
	.verify	= sw_verify,
};

U_BOOT_DRIVER(xmss_engine_sw) = {
	.name	= "xmss_engine_sw",
	.id	= UCLASS_XMSS_ENGINE,
	.ops	= &ops,
	.flags	= DM_FLAG_PRE_RELOC,
};

U_BOOT_DEVICE(xmss_engine_sw) = {
	.name = "xmss_engine_sw",
};
