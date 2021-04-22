/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 Intel Corporation
 */

/* CPUSS reset ID's */
#define TBH_CPUSS_OCS_RST                             1

/* PSS reset ID's */
#define TBH_PSS_EFUSE_PRSTN                           1
#define TBH_PSS_GPIO_RST_N                            2
#define TBH_PSS_I2C_INST_0_RST_N                      3
#define TBH_PSS_I2C_INST_1_RST_N                      4
#define TBH_PSS_I2C_INST_2_RST_N                      5
#define TBH_PSS_I2C_INST_3_RST_N                      6
#define TBH_PSS_I2C_INST_4_RST_N                      7
#define TBH_PSS_UART_INST_0_RST_N                     8
#define TBH_PSS_UART_INST_1_RST_N                     9
#define TBH_PSS_EMMC_RST_N                            10

#define TBH_PSS_SSI_INST_0_RST_N                      1
#define TBH_PSS_SSI_INST_1_RST_N                      2
#define TBH_PSS_SMBUS_RST_N                           3
#define TBH_PSS_DMAC_INST_0_RST_N                     4
#define TBH_PSS_TRNG_RST_N                            5
#define TBH_PSS_ETHERNET_INST_0_RST_N                 6
#define TBH_PSS_ETHERNET_INST_1_RST_N                 7
#define TBH_PSS_DMAC_SECURE_RST_N                     8

/* PCIe reset ID's */
#define TBH_PCIE_CTRL1_RST                            1
#define TBH_PCIE_CTRL0_RST                            2
#define TBH_PCIE_PHY_RST                              3
#define TBH_PCIE_SUBSYSTEM_RST                        4
#define TBH_PCIE_POWER_ON_RST                         5

/* COMSS reset ID's */
#define TBH_COMSS_VPU_VPU_RESET_N                     1
#define TBH_COMSS_OCS_OCS_RESET_N                     2
#define TBH_COMSS_MEDIA_VCEJ_CORERST_N                3
#define TBH_COMSS_MEDIA_VCEJ_ARESET_N                 4
#define TBH_COMSS_MEDIA_VCEJ_PRESET_N                 5
#define TBH_COMSS_MEDIA_VCE_CORERST_N                 6
#define TBH_COMSS_MEDIA_VCE_ARESET_N                  7
#define TBH_COMSS_MEDIA_VCE_PRESET_N                  8
#define TBH_COMSS_MEDIA_VCDA_CORERST_N                9
#define TBH_COMSS_MEDIA_VCDA_ARESET_N                 10
#define TBH_COMSS_MEDIA_VCDA_PRESET_N                 11
#define TBH_COMSS_MEDIA_VCDB_CORERST_N                12
#define TBH_COMSS_MEDIA_VCDB_ARESET_N                 13
#define TBH_COMSS_MEDIA_VCDB_PRESET_N                 14
#define TBH_COMSS_MEDIA_TBU0_ARESET_N                 15
#define TBH_COMSS_MEDIA_TBU1_ARESET_N                 16
#define TBH_COMSS_MEDIA_TBU2_ARESET_N                 17
#define TBH_COMSS_MEDIA_TBU3_ARESET_N                 18
#define TBH_COMSS_MEDIA_TBU4_ARESET_N                 19
#define TBH_COMSS_MEDIA_TCU_ARESET_N                  20
#define TBH_COMSS_MEDIA_DTI_ARESET_N                  21
#define TBH_COMSS_MEDIA_PVT_ARESET_N                  22
#define TBH_COMSS_MEDIA_PVT_PRESET_N                  23

/* MEMSS reset ID's */
# define TBH_MEMSS_MC_MC_U1_CORE_DDRC_RSTN            1
# define TBH_MEMSS_MC_MC_U2_CORE_DDRC_RSTN            2
# define TBH_MEMSS_MC_MC_U1_SBR_RESETN                3
# define TBH_MEMSS_MC_MC_U2_SBR_RESETN                4
# define TBH_MEMSS_MC_MC_U1_ARESETN_0                 5
# define TBH_MEMSS_MC_MC_U2_ARESETN_0                 6
# define TBH_MEMSS_MC_MC_U1_PRESETN                   7
# define TBH_MEMSS_MC_MC_U2_PRESETN                   8
# define TBH_MEMSS_MC_MC_U1_SCAN_RESETN               9
# define TBH_MEMSS_MC_MC_U2_SCAN_RESETN               10
# define TBH_MEMSS_MC_PHY_U1_PRESETN_APB              11
# define TBH_MEMSS_MC_PHY_U1_PWROKIN                  12
# define TBH_MEMSS_MC_PHY_U1_RESET                    13
# define TBH_MEMSS_MC_PHY_U1_WRSTN                    14
# define TBH_MEMSS_MC_DDRSS_APB_REGS_U1_I_PRESETN     15
# define TBH_MEMSS_MC_DDRSS_APB_REGS_U1_I_RSTN        16
# define TBH_MEMSS_MC_MC_COUNTER_TOP_U1_I_MCA_RSTN    17
# define TBH_MEMSS_MC_MC_COUNTER_TOP_U1_I_MCB_RSTN    18
# define TBH_MEMSS_MC_DDRSS_GLUE_U1_I_MCA_RSTN        19
# define TBH_MEMSS_MC_DDRSS_GLUE_U1_I_MCB_RSTN        20
# define TBH_MEMSS_PHY_PHY_U1_PRESETN_APB             21
# define TBH_MEMSS_PHY_PHY_U1_PWROKIN                 22
# define TBH_MEMSS_PHY_PHY_U1_RESET                   23
# define TBH_MEMSS_PHY_PHY_U1_WRSTN                   24
# define TBH_MEMSS_MISC_DDRSS_APB_REGS_U1_I_PRESETN   25
# define TBH_MEMSS_MISC_DDRSS_APB_REGS_U1_I_RSTN      26
# define TBH_MEMSS_MISC_MC_COUNTER_TOP_U1_I_MCA_RSTN  27
# define TBH_MEMSS_MISC_MC_COUNTER_TOP_U1_I_MCB_RSTN  28
# define TBH_MEMSS_MISC_DDRSS_GLUE_U1_I_MCA_RSTN      29
# define TBH_MEMSS_MISC_DDRSS_GLUE_U1_I_MCB_RSTN      30
