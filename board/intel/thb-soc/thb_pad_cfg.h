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

struct pcl_pad_control {
	u32 index; /* Pin Number */
	u32 value; /* Desired value to be configured */
};

/*
 * pcl_pad_config() - Apply GPIO pad configuration based on board and version(full or prime)
 */
void pcl_pad_config(u8 board_id, u8 thb_full);

/*
 * pcl_pad_write_config() - Write the configuration to the GPIO pad
 */
void pcl_pad_write_config(const struct pcl_pad_control *config_pad, u32 pad_size);
