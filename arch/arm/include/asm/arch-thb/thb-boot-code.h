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

#define PCIE_CPR_BASE                            (0x82800000)
#define PCIE_CPR_PCIE_RST_EN_OFFSET              (0x0000)
#define PCIE_CPR_PCIE_RST_EN                     (PCIE_CPR_BASE + PCIE_CPR_PCIE_RST_EN_OFFSET)

#define CLKSS_PCIE_BASE                          (0x82806000)
#define CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_45_OFFSET (0x00b4)
#define CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_46_OFFSET (0x00b8)
#define CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_45        (CLKSS_PCIE_BASE + CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_45_OFFSET)
#define CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_46        (CLKSS_PCIE_BASE + CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_46_OFFSET)

#define PSS_CPR_BASE                             (0x80430000)
#define PSS_CPR_ETHERNET_INST0_RST_EN_OFFSET     (0x16000)
#define PSS_CPR_ETHERNET_INST1_RST_EN_OFFSET     (0x17000)
#define PSS_CPR_MISC_CLK_REG_0_OFFSET            (0x6810)
#define PSS_CPR_EMMC_RST_EN_OFFSET               (0x10000)
#define PSS_CPR_EMMC_CLK_GATING_OFFSET           (0x10400)
#define PSS_CPR_ETHERNET_INST0_RST_EN            (PSS_CPR_BASE + PSS_CPR_ETHERNET_INST0_RST_EN_OFFSET)
#define PSS_CPR_ETHERNET_INST1_RST_EN            (PSS_CPR_BASE + PSS_CPR_ETHERNET_INST1_RST_EN_OFFSET)
#define PSS_CPR_MISC_CLK_REG_0                   (PSS_CPR_BASE + PSS_CPR_MISC_CLK_REG_0_OFFSET)
#define PSS_CPR_EMMC_RST_EN                      (PSS_CPR_BASE + PSS_CPR_EMMC_RST_EN_OFFSET)
#define PSS_CPR_EMMC_CLK_GATING                  (PSS_CPR_BASE + PSS_CPR_EMMC_CLK_GATING_OFFSET)

#define CLKSS_ETH_BASE				 (0x80436200)
#define CLKSS_ETH_APB_CTRL_20_OFFSET		 (0x50)
#define CLKSS_ETH_APB_CTRL_21_OFFSET		 (0x54)
#define CLKSS_ETH_APB_CTRL_22_OFFSET             (0x58)
#define CLKSS_ETH_APB_CTRL_23_OFFSET             (0x5C)
#define CLKSS_ETH_APB_CTRL_24_OFFSET             (0x60)
#define CLKSS_ETH_APB_CTRL_25_OFFSET             (0x64)
#define CLKSS_ETH_APB_CTRL_20			 (CLKSS_ETH_BASE + CLKSS_ETH_APB_CTRL_20_OFFSET)
#define CLKSS_ETH_APB_CTRL_21                    (CLKSS_ETH_BASE + CLKSS_ETH_APB_CTRL_21_OFFSET)
#define CLKSS_ETH_APB_CTRL_22                    (CLKSS_ETH_BASE + CLKSS_ETH_APB_CTRL_22_OFFSET)
#define CLKSS_ETH_APB_CTRL_23                    (CLKSS_ETH_BASE + CLKSS_ETH_APB_CTRL_23_OFFSET)
#define CLKSS_ETH_APB_CTRL_24                    (CLKSS_ETH_BASE + CLKSS_ETH_APB_CTRL_24_OFFSET)
#define CLKSS_ETH_APB_CTRL_25                    (CLKSS_ETH_BASE + CLKSS_ETH_APB_CTRL_25_OFFSET)

extern u8 board_id;

#endif /* _THB_BOOT_CODE_H */
