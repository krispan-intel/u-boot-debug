/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019-2020 Intel Corporation.
 */
#ifndef _THB_BOOT_CODE_H
#define _THB_BOOT_CODE_H

#define SECURITY_FAIL_EMMC_WP (0x1)
#define SECURITY_FAIL_SPI_WP (0x2)
#define SECURITY_FAIL_TPM_INIT (0x3)
#define SECURITY_FAIL_TPM_DEINIT (0x4)
#define SECURITY_FAIL_TPM_CHANGE_PLAT_HIER (0x5)
#define SECURITY_FAIL_BL2_MEASUREMENT (0x6)
#define SECURITY_FAIL_OS_MEASUREMENT (0x7)
#define SECURITY_FAIL_IMR_PREBOOT_OS (0x8)
#define SECURITY_FAIL_IMR_BOOTM_START (0x9)

extern u8 board_id;
#endif /* _THB_BOOT_CODE_H */
