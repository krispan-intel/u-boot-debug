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

#include "pcl_internal_config.h"

const struct pcl_pad_control evt2_full_pad[] = {
	/* PCL_I2C0_SCL */
	{ 0, // GPIO_0
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_OD
		 | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C0_SDA */
	{ 1, // GPIO_1
		PCL_MODE_IS_0 | PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG
		 | PCL_DP_IN | PCL_OD | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C1_SCL */
	{ 2, // GPIO_2
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_OD
		 | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C1_SDA */
	{ 3, // GPIO_3
		PCL_MODE_IS_0 | PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG
		 | PCL_DP_IN | PCL_OD | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_TBH1_TPM_GPIO */
	{ 4, // GPIO_4
		PCL_MODE_IS_4 | PCL_MODE_OS_4 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_GPIO | PCL_DP_IN
		 | PCL_OD | PCL_PU | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_CMOS
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_TBH1_BRD_FLT_LED */
	{ 5, // GPIO_5
		PCL_MODE_OS_4 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_GPIO | PCL_DP_OUT | PCL_OD
		 | PCL_PU | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_CMOS
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C3_SCL */
	{ 6, // GPIO_6
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_OD
		 | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C3_SDA */
	{ 7, // GPIO_7
		PCL_MODE_IS_0 | PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG
		 | PCL_DP_IN | PCL_OD | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_GPIO_EXP_INT_N */
	{ 8, // GPIO_8
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
		 | PCL_MODEI2CNCMOS_CMOS | PCL_MODE1P8N1P2B_0
	},
	/* PCL_TBH1 GPIO9_TBH_MEM_TYPE2 */
	{ 9, // GPIO_9
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
		 | PCL_MODEI2CNCMOS_CMOS | PCL_MODE1P8N1P2B_0
	},
	/* PCL_UART0_SOUT */
	{ 10, // GPIO_10
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_UART0_SIN */
	{ 11, // GPIO_11
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_MEM_TYPE0 */
	{ 12, // GPIO_12
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_MEM_TPYE1 */
	{ 13, // GPIO_13
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH1_EFUSE_SW_EN */
	{ 14, // GPIO_14
		PCL_MODE_OS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBH_BOARD_ID3 */
	{ 15, // GPIO_15
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_BOARD_ID2 */
	{ 16, // GPIO_16
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_BOARD_ID1 */
	{ 17, // GPIO_17
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_THERM_SENSOR_ALERT1 */
	{ 18, // GPIO_18
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_BOARD_ID0 */
	{ 19, // GPIO_19
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH1 AUX_12V_Sense0 */
	{ 21, // GPIO_21
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH1_SPI1_TPM_SCLK */
	{ 22, // GPIO_22
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBH1_SPI1_TPM_SS_N */
	{ 23, // GPIO_23
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBH1_SPI1_TPM_MOSI */
	{ 24, // GPIO_24
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBH1_SPI1_TPM_MISO */
	{ 25, // GPIO_25
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_PWR_ADS_BUSY */
	{ 26, // GPIO_26
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBHP_GP35_SENSE0_V1P8_AUX */
	{ 35, // GPIO_35
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_POWER_INTERRUPT_MAX_PLATFORM_POWER */
	{ 41, // GPIO_41
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TPM_IRQ_N */
	{ 56, // GPIO_56
		PCL_MODE_IS_3 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_POWER_INTERRUPT_ICCMAX_VPU */
	{ 57, // GPIO_57
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_THERMTRIP_IN */
	{ 58, // GPIO_58
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_THERMTRIP_OUT */
	{ 59, // GPIO_59
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_SMBUS_SCL */
	{ 60, // GPIO_60
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_OD | PCL_ENAQ_EN
	},
	/* PCL_SMBUS_SDA */
	{ 61, // GPIO_61
		PCL_MODE_IS_0 | PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN
		 | PCL_OD | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_PLATFORM_RESET_IN */
	{ 62, // GPIO_62
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_PLATFORM_RESET_OUT */
	{ 63, // GPIO_63
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_PLATFORM_SHUTDOWN_IN */
	{ 64, // GPIO_64
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_PLATFORM_SHUTDOWN_OUT */
	{ 65, // GPIO_65
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_POWER_INTERRUPT_ICCMAX_MEDIA */
	{ 66, // GPIO_66
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	}
};

const struct pcl_pad_control evt2_prime_pad[] = {
	/* PCL_I2C0_SCL */
	{ 0, // GPIO_0
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_OD
		 | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C0_SDA */
	{ 1, // GPIO_1
		PCL_MODE_IS_0 | PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG
		 | PCL_DP_IN | PCL_OD | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C1_SCL */
	{ 2, // GPIO_2
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_OD
		 | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C1_SDA */
	{ 3, // GPIO_3
		PCL_MODE_IS_0 | PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG
		 | PCL_DP_IN | PCL_OD | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_TBHP_BRD_FLT_LED */
	{ 5, // GPIO_5
		PCL_MODE_OS_4 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_GPIO | PCL_DP_OUT | PCL_OD
		 | PCL_PU | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_CMOS
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C3_SCL */
	{ 6, // GPIO_6
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_OD
		 | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C | PCL_MODE1P8N1P2B_0
	},
	/* PCL_I2C3_SDA */
	{ 7, // GPIO_7
		PCL_MODE_IS_0 | PCL_MODE_OS_0 | PCL_ID_EN | PCL_FBD_ON | PCL_PS_INTERNAL_SIG
		 | PCL_DP_IN | PCL_OD | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW | PCL_MODEI2CNCMOS_I2C
		 | PCL_MODE1P8N1P2B_0
	},
	/* PCL_GPIO_EXP_INT_N */
	{ 8, // GPIO_8
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
		 | PCL_MODEI2CNCMOS_CMOS | PCL_MODE1P8N1P2B_0
	},
	/* PCL_TBHP GPIO9_TBH_MEM_TYPE2 */
	{ 9, // GPIO_9
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
		 | PCL_MODEI2CNCMOS_CMOS | PCL_MODE1P8N1P2B_0
	},
	/* PCL_UART0_SOUT */
	{ 10, // GPIO_10
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_UART0_SIN */
	{ 11, // GPIO_11
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBHP_MEM_TYPE0 */
	{ 12, // GPIO_12
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBHP_MEM_TPYE1 */
	{ 13, // GPIO_13
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBHP_EFUSE_SW_EN */
	{ 14, // GPIO_14
		PCL_MODE_OS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBH_BOARD_ID3 */
	{ 15, // GPIO_15
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_BOARD_ID2 */
	{ 16, // GPIO_16
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_BOARD_ID1 */
	{ 17, // GPIO_17
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_THERM_SENSOR_ALERT1 */
	{ 18, // GPIO_18
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_BOARD_ID0 */
	{ 19, // GPIO_19
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBHP AUX_12V_Sense0 */
	{ 21, // GPIO_21
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBHP_SPI1_TPM_SCLK */
	{ 22, // GPIO_22
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBHP_SPI1_TPM_SS_N */
	{ 23, // GPIO_23
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBHP_SPI1_TPM_MOSI */
	{ 24, // GPIO_24
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBHP_SPI1_TPM_MISO */
	{ 25, // GPIO_25
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBH_PWR_ADS_BUSY */
	{ 26, // GPIO_26
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TBHP_TPM_GPIO */
	{ 32, // GPIO_32
		PCL_MODE_IS_4 | PCL_MODE_OS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_PP
		 | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_TBHP_GP35_SENSE0_V1P8_AUX */
	{ 35, // GPIO_35
		PCL_MODE_IS_4 | PCL_ID_EN | PCL_PS_GPIO | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_POWER_INTERRUPT_MAX_PLATFORM_POWER */
	{ 41, // GPIO_41
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_TPM_IRQ_N */
	{ 56, // GPIO_56
		PCL_MODE_IS_3 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_POWER_INTERRUPT_ICCMAX_VPU */
	{ 57, // GPIO_57
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_THERMTRIP_IN */
	{ 58, // GPIO_58
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_THERMTRIP_OUT */
	{ 59, // GPIO_59
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_SMBUS_SCL */
	{ 60, // GPIO_60
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_OD | PCL_ENAQ_EN
	},
	/* PCL_SMBUS_SDA */
	{ 61, // GPIO_61
		PCL_MODE_IS_0 | PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN
		 | PCL_OD | PCL_ENAQ_EN | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_PLATFORM_RESET_IN */
	{ 62, // GPIO_62
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_PLATFORM_RESET_OUT */
	{ 63, // GPIO_63
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_PLATFORM_SHUTDOWN_IN */
	{ 64, // GPIO_64
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	},
	/* PCL_PLATFORM_SHUTDOWN_OUT */
	{ 65, // GPIO_65
		PCL_MODE_OS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_OUT | PCL_PP | PCL_ENAQ_EN
		 | PCL_DRV_7 | PCL_SLOW
	},
	/* PCL_POWER_INTERRUPT_ICCMAX_MEDIA */
	{ 66, // GPIO_66
		PCL_MODE_IS_0 | PCL_ID_EN | PCL_PS_INTERNAL_SIG | PCL_DP_IN | PCL_ENAQ_EN
	}
};