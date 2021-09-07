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

/* Macro to define Input Selection mode of the pad */
#define PCL_LSB_MODE_IS	(0x0)

/* Macros to define mode configurations for input selection (0-2 bits) of the pad */
#define PCL_MODE_IS_0	(0x0 << PCL_LSB_MODE_IS)
#define PCL_MODE_IS_1	(0x1 << PCL_LSB_MODE_IS)
#define PCL_MODE_IS_2	(0x2 << PCL_LSB_MODE_IS)
#define PCL_MODE_IS_3	(0x3 << PCL_LSB_MODE_IS)
#define PCL_MODE_IS_4	(0x4 << PCL_LSB_MODE_IS)

/* Macros to define Input Disable configuration of the pad (3 bit) */
#define PCL_LSB_ID	(0x3)
#define PCL_ID_EN	(0x0 << PCL_LSB_ID)
#define PCL_ID_DIS	(0x1 << PCL_LSB_ID)

/* Macro to define Output Selection mode of the pad */
#define PCL_LSB_MODE_OS	(0x4)

/* Macros to define mode configurations for output selection (4-6 bits) of the pad */
#define PCL_MODE_OS_0	(0x0 << PCL_LSB_MODE_OS)
#define PCL_MODE_OS_1	(0x1 << PCL_LSB_MODE_OS)
#define PCL_MODE_OS_2	(0x2 << PCL_LSB_MODE_OS)
#define PCL_MODE_OS_3	(0x3 << PCL_LSB_MODE_OS)
#define PCL_MODE_OS_4	(0x4 << PCL_LSB_MODE_OS)

/* Macros to define Feedback Disable configuration of the pad (7 bit) */
#define PCL_LSB_FBD	(0x7)
#define PCL_FBD_ON	(0x0 << PCL_LSB_FBD)
#define PCL_FBD_OFF	(0x1 << PCL_LSB_FBD)

/* Macros to define Port Select configuration of the pad (8 bit) */
#define PCL_LSB_PS		(0x8)
#define PCL_PS_INTERNAL_SIG	(0x0 << PCL_LSB_PS)
#define PCL_PS_GPIO		(0x1 << PCL_LSB_PS)

/* Macros to define direction configurations of the pad */
#define PCL_LSB_DP	(0xa)	// The bit for direction of pad is 10.
#define PCL_DP_I	(0x0)
#define PCL_DP_O	(0x1)

/* Macro to configure direction to input of the pad */
#define PCL_DP_IN	(PCL_DP_I << PCL_LSB_DP)

/* Macro to configure direction to output of the pad */
#define PCL_DP_OUT	(PCL_DP_O << PCL_LSB_DP)

/* Macros to define strong pull function configurations of the pad (enable and disable) */
#define PCL_LSB_SPU	(0xb)	// The bit for pull type of pad is 11.
#define PCL_SPU_DIS	(0x0 << PCL_LSB_SPU)
#define PCL_SPU_EN	(0x1 << PCL_LSB_SPU)

/* Macros to define drive type configurations for 2 values of the pad */
#define PCL_LSB_PPEN	(0xc)	// The bit for drive type of pad is 12.
#define PCL_PPEN_OD	(0x0)
#define PCL_PPEN_PP	(0x1)

/* Macro to activate Open drain function of the pad */
#define PCL_OD		(PCL_PPEN_OD << PCL_LSB_PPEN)

/* Macro to activate Push/pull function of the pad */
#define PCL_PP		(PCL_PPEN_PP << PCL_LSB_PPEN)

/* Macros to define Pull up function of the pad (13 bit) */
#define PCL_LSB_PUQ	(0xd)
#define PCL_PUQ_EN	(0x0)
#define PCL_PUQ_DIS	(0x1)

#define PCL_PU		(PCL_PUQ_EN << PCL_LSB_PUQ)
#define PCL_PU_DIS	(PCL_PUQ_DIS << PCL_LSB_PUQ)

/* Macros to define Pull down function of the pad (14 bit) */
#define PCL_LSB_PD	(0xe)
#define PCL_PD_EN	(0x1)
#define PCL_PD_DIS	(0x0 << PCL_LSB_PD)

#define PCL_PD		(PCL_PD_EN << PCL_LSB_PD)
//#define PCL_PD_DIS	(PCL_PD_DIS << PCL_LSB_PD)

/* Macros to define output function of the pad */
#define PCL_LSB_ENAQ	(0xf)
#define PCL_ENAQ_EN	(0x0 << PCL_LSB_ENAQ)
#define PCL_ENAQ_DIS	(0x1 << PCL_LSB_ENAQ)

/* Macros to define drive strength configurations for the pad */
#define PCL_LSB_DRV	(0x10)	// The bit for drive strength of pad is 16-19.

#define PCL_DRV_0	(PCL_DRV_DRVP << PCL_LSB_DRV)
#define PCL_DRV_1	(PCL_DRV_DRVO << PCL_LSB_DRV)
#define PCL_DRV_2	(PCL_DRV_DRVN << PCL_LSB_DRV)
#define PCL_DRV_3	(PCL_DRV_DRVM << PCL_LSB_DRV)
#define PCL_DRV_4	(PCL_DRV_DRVL << PCL_LSB_DRV)
#define PCL_DRV_5	(PCL_DRV_DRVK << PCL_LSB_DRV)
#define PCL_DRV_6	(PCL_DRV_DRVJ << PCL_LSB_DRV)
#define PCL_DRV_7	(PCL_DRV_DRVI << PCL_LSB_DRV)
#define PCL_DRV_8	(PCL_DRV_DRVH << PCL_LSB_DRV)
#define PCL_DRV_9	(PCL_DRV_DRVG << PCL_LSB_DRV)
#define PCL_DRV_10	(PCL_DRV_DRVF << PCL_LSB_DRV)
#define PCL_DRV_11	(PCL_DRV_DRVE << PCL_LSB_DRV)
#define PCL_DRV_12	(PCL_DRV_DRVD << PCL_LSB_DRV)
#define PCL_DRV_13	(PCL_DRV_DRVC << PCL_LSB_DRV)
#define PCL_DRV_14	(PCL_DRV_DRVB << PCL_LSB_DRV)
#define PCL_DRV_15	(PCL_DRV_DRVA << PCL_LSB_DRV)

#define PCL_DRV_DRVP	(0x0)
#define PCL_DRV_DRVO	(0X1)
#define PCL_DRV_DRVN	(0x2)
#define PCL_DRV_DRVM	(0x3)
#define PCL_DRV_DRVL	(0x4)
#define PCL_DRV_DRVK	(0x5)
#define PCL_DRV_DRVJ	(0x6)
#define PCL_DRV_DRVI	(0x7)
#define PCL_DRV_DRVH	(0x8)
#define PCL_DRV_DRVG	(0x9)
#define PCL_DRV_DRVF	(0xa)
#define PCL_DRV_DRVE	(0xb)
#define PCL_DRV_DRVD	(0xc)
#define PCL_DRV_DRVC	(0xd)
#define PCL_DRV_DRVB	(0xe)
#define PCL_DRV_DRVA	(0xf)

/* Macros to define slew rate configurations for the pad */
#define PCL_LSB_SR	(0x14)	// The bit for slew rate of pad is 20.
#define PCL_SR_SRL	(0x0)
#define PCL_SR_SRH	(0x1)

/* Macro to set the slew rate as slow for the pad */
#define PCL_SLOW	(PCL_SR_SRL << PCL_LSB_SR)

/* Macro to set the slew rate as high for the pad */
#define PCL_FAST	(PCL_SR_SRH << PCL_LSB_SR)

/* Macros to define the I2C or CMOS mode for the pad */
#define PCL_LSB_MODEI2CNCMOS	(0x14)
#define PCL_MODEI2CNCMOS_I2C	(0x1 << PCL_LSB_MODEI2CNCMOS)
#define PCL_MODEI2CNCMOS_CMOS	(0x0 << PCL_LSB_MODEI2CNCMOS)

/* Macros to define the I2C Volt value for the pad */
#define PCL_LSB_MODE1P8N1P2B	(0x15)
#define PCL_MODE1P8N1P2B_0	(0x0 << PCL_LSB_MODE1P8N1P2B)
#define PCL_MODE1P8N1P2B_1	(0x1 << PCL_LSB_MODE1P8N1P2B)

/* Macros to define schmitt trigger configurations (21 bit) for the pad */
#define PCL_LSB_ST	(0x15)
#define PCL_ST_EN	(0x1 << PCL_LSB_ST)
#define PCL_ST_DIS	(0x0 << PCL_ST_DIS)
