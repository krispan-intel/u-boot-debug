/*
 * Copyright (c) 2019-2020 Intel Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 * Defines for access control.
 */

#ifndef __ACCESS_CONTROL_H__
#define __ACCESS_CONTROL_H__

#define NUM_IMR_INIT                                    ( 50 )

/* IMR initiator secure IDs. */
#define FIREWALL_DEC400_INIT_SECURE_ID                  ( 45 )
#define FIREWALL_UPA_L2C_INIT_SECURE_ID                 ( 46 )
#define FIREWALL_MMU500_TCU_INIT_SECURE_ID              ( 42 )
#define FIREWALL_NCE_MBI_TARG_INIT_SECURE_ID            ( 44 )
#define FIREWALL_AMC_INIT_SECURE_ID                     ( 1 )
#define FIREWALL_CIF_INIT_SECURE_ID                     ( 2 )
#define FIREWALL_LCD_INIT_SECURE_ID                     ( 3 )
#define FIREWALL_MIPI_RX_0_INIT_SECURE_ID               ( 4 )
#define FIREWALL_MIPI_RX_1_INIT_SECURE_ID               ( 5 )
#define FIREWALL_MIPI_RX_2_INIT_SECURE_ID               ( 6 )
#define FIREWALL_MIPI_RX_3_INIT_SECURE_ID               ( 7 )
#define FIREWALL_MIPI_RX_4_INIT_SECURE_ID               ( 8 )
#define FIREWALL_MIPI_RX_5_INIT_SECURE_ID               ( 9 )
#define FIREWALL_MIPI_TX_0_INIT_SECURE_ID               ( 10 )
#define FIREWALL_MIPI_TX_1_INIT_SECURE_ID               ( 11 )
#define FIREWALL_MIPI_TX_2_INIT_SECURE_ID               ( 12 )
#define FIREWALL_MIPI_TX_3_INIT_SECURE_ID               ( 13 )
#define FIREWALL_SLVDS0_INIT_SECURE_ID                  ( 14 )
#define FIREWALL_SLVDS1_INIT_SECURE_ID                  ( 15 )
#define FIREWALL_LEON_MSS_RT_L2C_INIT_SECURE_ID         ( 16 )
#define FIREWALL_LEON_MSS_RT_AHB_BP_INIT_SECURE_ID      ( 17 )
#define FIREWALL_DTB_DCT_FFT_INIT_SECURE_ID             ( 19 )
#define FIREWALL_BLT_INIT_SECURE_ID                     ( 20 )
#define FIREWALL_VDEC_INIT_SECURE_ID                    ( 21 )
#define FIREWALL_VENC_INIT_SECURE_ID                    ( 22 )
#define FIREWALL_A53_CPU_SECURE_INIT_SECURE_ID          ( 48 )
#define FIREWALL_A53_CPU_UNSECURE_INIT_SECURE_ID        ( 49 )
#define FIREWALL_AXIDMA_INIT_SECURE_ID                  ( 24 )
#define FIREWALL_EMMC_INIT_SECURE_ID                    ( 25 )
#define FIREWALL_GIGE_INIT_SECURE_ID                    ( 26 )
#define FIREWALL_JPEG_INIT_SECURE_ID                    ( 27 )
#define FIREWALL_DBG_ULTRASOC_SECURE_INIT_SECURE_ID     ( 28 )
#define FIREWALL_DBG_ULTRASOC_UNSECURE_INIT_SECURE_ID   ( 29 )
#define FIREWALL_DBG_JTAG_SECURE_INIT_SECURE_ID         ( 30 )
#define FIREWALL_DBG_JTAG_UNSECURE_INIT_SECURE_ID       ( 31 )
#define FIREWALL_DBG_TRACE_SECURE_INIT_SECURE_ID        ( 32 )
#define FIREWALL_DBG_TRACE_UNSECURE_INIT_SECURE_ID      ( 33 )
#define FIREWALL_OCS_AES_INIT_SECURE_ID                 ( 34 )
#define FIREWALL_OCS_HCU_INIT_SECURE_ID                 ( 35 )
#define FIREWALL_PCIE_INIT_SECURE_ID                    ( 36 )
#define FIREWALL_SDIO_HOST_0_INIT_SECURE_ID             ( 37 )
#define FIREWALL_SDIO_HOST_1_INIT_SECURE_ID             ( 38 )
#define FIREWALL_USB_INIT_SECURE_ID                     ( 39 )
#define FIREWALL_UPA_CMX_M_INIT_SECURE_ID               ( 40 )
#define FIREWALL_UPA_CMX_DMA_INIT_SECURE_ID             ( 41 )

#endif /* __ACCESS_CONTROL_H__ */
