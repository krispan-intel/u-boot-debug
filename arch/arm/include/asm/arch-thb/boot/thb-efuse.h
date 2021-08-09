/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */
/** efuse array bits. */
#define MA_EFUSE_NUM_BITS (8192 + 4096)  /* 8K+4K Bits */

int efuse_read_ranges(u32 * const boot_operation, const u32 start_u32_idx,
			     const u32 end_u32_idx);

int efuse_write_ranges(u32 * const boot_operation, u32 * const fuse_mask, const u32 start_u32_idx,
			      const u32 end_u32_idx, const u32 flags);

