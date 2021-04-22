/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020 Intel Corporation
 */

#ifndef __THB_SHARED_MEM_H__
#define __THB_SHARED_MEM_H__

#include <asm/arch/boot/platform_private.h>

/**
 * get_secure_shmem_ptr() - Get a shared memory pointer from device tree.
 * @nodename: Name of the shared memory node
 * @min_size: Minimum size we would require
 *
 * Retrieve an area that can be shared with the secure world for
 * communication.
 *
 * Return: pointer, or NULL when failed.
 */
void __iomem *get_secure_shmem_ptr(size_t min_size);

/**
 * get_bl1_ctx() - Get BL1 context.
 *
 * Get the BL1 context (which includes various information, such as the boot
 * interface used by BL1 and the USB configuration) from the runtime monitor.
 *
 * Return: pointer to BL1 context, NULL pointer otherwise.
 */
int get_bl1_ctx(platform_bl1_ctx_t *bl1_ctx);

/**
 * get_bl_ctx() - Get BL context.
 *
 * Query the platform BL context, which includes various information,
 * include firmware hash algo used in ROM and the digests of secure world
 * and normal world.
 *
 * Return: pointer to BL context, NULL pointer otherwise.
 */
//platform_bl_ctx_t *get_bl_ctx(void);

#endif /* __THB_SHARED_MEM_H__ */
