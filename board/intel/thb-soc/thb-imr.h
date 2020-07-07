/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019-2020 Intel Corporation
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

/**
 * thb_imr_post_u_boot_reloc() - Perform post U-Boot relocation IMR setup.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int thb_imr_post_u_boot_reloc(void);

/**
 * thb_imr_bootm_start() - Perform IMR setup required at bootm start.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int thb_imr_bootm_start(void);

/**
 * thb_imr_preboot_os() - Perform pre-boot IMR setup.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int thb_imr_preboot_os(void);
