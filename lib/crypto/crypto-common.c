// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (R) 2019 Intel Corporation
 */

#include "crypto-common.h"

#ifndef USE_HOSTCC
#include <common.h>
#else /* USE_HOSTCC */
#include "mkimage.h"
#endif /* USE_HOSTCC */

/* See documentation in header file. */
int crypto_verify(struct image_sign_info *info,
		  const struct image_region region[], int region_count,
		  uint8_t *sig, uint sig_len, verify_fn_t verify_fn)
{
	const void *blob = info->fdt_blob;
	const size_t hash_len = info->checksum->checksum_len;
	/* Reserve memory for checksum-length */
	uint8_t hash[hash_len];
	int ndepth, noffset;
	int sig_node, node;
	char name[100];
	int ret;

	/* Get signature FDT node. */
	sig_node = fdt_subnode_offset(blob, 0, FIT_SIG_NODENAME);
	if (sig_node < 0) {
		debug("%s: No signature node found\n", __func__);
		return -ENOENT;
	}

	/* Calculate checksum with checksum-algorithm */
	ret = info->checksum->calculate(info->checksum->name,
					region, region_count, hash);
	if (ret < 0) {
		debug("%s: Error in checksum calculation\n", __func__);
		return -EINVAL;
	}

	/* See if we must use a particular key */
	if (info->required_keynode != -1) {
		ret = verify_fn(info, hash, hash_len, sig, sig_len,
				info->required_keynode);
		return ret;
	}

	/* Look for a key that matches our hint */
	snprintf(name, sizeof(name), "key-%s", info->keyname);
	node = fdt_subnode_offset(blob, sig_node, name);
	if (node >= 0) {
		ret = verify_fn(info, hash, hash_len, sig, sig_len, node);
		if (!ret)
			return ret;
	}

	/* No luck, so try each of the keys in turn */
	for (ndepth = 0, noffset = fdt_next_node(info->fit, sig_node, &ndepth);
			(noffset >= 0) && (ndepth > 0);
			noffset = fdt_next_node(info->fit, noffset, &ndepth)) {
		if (ndepth == 1 && noffset != node) {
			ret = verify_fn(info, hash, hash_len, sig, sig_len,
					noffset);
			if (!ret)
				break;
		}
	}

	return ret;
}
