/*
 * Copyright (c) 2018, Intel Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/types.h>

/**
 * Setup up dm-verity boot args.
 *
 * Extract dm-verity root-hash from a Linux aarch64 Image (if present) and set
 * boot args related to dm-verity (i.e., 'verity=<bool>' and 'roothash=<hash>').
 *
 * If SECURE_SKU == 1 and roothash is present, bootargs are updated with
 * "verity=1 roothash=<extracted_hash>"; otherwise if SECURE_SKU == 0 *or* no
 * roothash is present, bootargs are updated with "verity=0".
 *
 * If 'bootargs' env already contains "verity" and/or "roothash" arguments,
 * they are removed / replaced.
 *
 * @param[in] image_addr The address where the Kernel Image is located.
 * @param[in] image_size The size of the image.
 * @param[in] full_scan  If true, dm-verity blob will be searched in the entire
 *                       image, starting from the end (this is needed when
 *                       'image_size' is overestimated, e.g., when called from
 *                       booti); if false, the dm-verity blob will be searched
 *                       only at the very end of the image.
 *
 * @return 0 on success ('bootargs' successfully updated), 1 on failure (e.g.,
 *           some internal error prevented 'bootargs' to be updated).
 */
int verity_setup_boot_args(ulong image, ulong image_size, bool full_scan);

