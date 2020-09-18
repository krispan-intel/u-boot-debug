/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020, Intel Corporation
 */

#ifndef _THB_SOC_MENDER_H_
#define _THB_SOC_MENDER_H_


#include <configs/thb-env.h>
#include <env_mender.h>

#define MENDER_BOOT_PART_NUMBER 	                4

#define MENDER_UBOOT_STORAGE_INTERFACE 	            "mmc"
#define MENDER_UBOOT_STORAGE_DEVICE 	            THB_SYS_MMC_ENV_DEV
#define MENDER_UBOOT_ENV_STORAGE_DEVICE_OFFSET_1 	THB_ENV_OFFSET
#define MENDER_UBOOT_ENV_STORAGE_DEVICE_OFFSET_2 	THB_ENV_OFFSET_REDUND

#define MENDER_STORAGE_DEVICE_BASE 		            "/dev/mmcblk0p"
#define MENDER_KERNEL_PART_A_NUMBER                 2
#define MENDER_KERNEL_PART_B_NUMBER                 3
#define MENDER_ROOTFS_PART_A_NUMBER 	            5
#define MENDER_ROOTFS_PART_B_NUMBER 	            8

#define MENDER_BOOTENV_SIZE 			            THB_ENV_SIZE

#define MENDER_BOOT_KERNEL_TYPE 		            "booti"
#define MENDER_KERNEL_NAME 				            "Image.bin"

#endif /* _THB_SOC_MENDER_H_ */
