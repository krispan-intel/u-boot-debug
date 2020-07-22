/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#ifndef __THB_PCIE_H__
#define __THB_PCIE_H__

#define IATU_DBI_OFFSET (0x300000)
#define IATU_OUTBOUND0_OFFSET (0x00)
#define IATU_INBOUND0_OFFSET (0x100)
#define IATU_INBOUND_REG_DIFF (0x200)
#define DMA_DBI_OFFSET (0x380000)
#define DMA_READ_CHAN0_OFFSET (0x300)
#define PCIE_CTRL_0_X2_USP_AXI_SLAVE_BASE        (0x2000000000)

/* PCIe DMA control 1 register definitions. */
#define PCIE_DMA_CH_CONTROL1_LIE_SHIFT (3)
#define PCIE_DMA_CH_CONTROL1_LIE_MASK (BIT(PCIE_DMA_CH_CONTROL1_LIE_SHIFT))

/* Read channel for boot. */
#define PCIE_DMA_COMMS_CHANNEL (0)

/* PCIe DMA Read Engine enable regsiter definitions. */
#define PCIE_DMA_READ_ENGINE_EN_SHIFT (0)
#define PCIE_DMA_READ_ENGINE_EN_MASK (BIT(PCIE_DMA_READ_ENGINE_EN_SHIFT))

/* PCIe DMA interrupt registers defintions. */
#define PCIE_DMA_ABORT_INTERRUPT_SHIFT (16)

#define PCIE_DMA_ABORT_INTERRUPT_MASK (0x7F << PCIE_DMA_ABORT_INTERRUPT_SHIFT)
#define PCIE_DMA_ABORT_INTERRUPT_CH_MASK(_c) (BIT(_c) << \
	PCIE_DMA_ABORT_INTERRUPT_SHIFT)
#define PCIE_DMA_DONE_INTERRUPT_MASK (0x7F)
#define PCIE_DMA_DONE_INTERRUPT_CH_MASK(_c) (BIT(_c))
#define PCIE_DMA_ALL_INTERRUPT_MASK (PCIE_DMA_ABORT_INTERRUPT_MASK | \
	PCIE_DMA_DONE_INTERRUPT_MASK)

/* PCIE IATU Control 2 register defines. */
#define PCIE_IATU_CTRL_2_BAR_NUM_SHIFT (8)
#define PCIE_IATU_CTRL_2_RESPONSE_TYPE_UNSUPPORTED (1)
#define PCIE_IATU_CTRL_2_RESPONSE_TYPE_SHIFT (24)
#define PCIE_IATU_CTRL_2_FUNC_NUM_MATCH_EN_SHIFT (19)
#define PCIE_IATU_CTRL_2_INVERTED_MODE_EN_SHIFT (29)
#define PCIE_IATU_CTRL_2_MATCH_MODE_SHIFT (30)
#define PCIE_IATU_CTRL_2_REGION_EN_SHIFT (31)
#define PCIE_IATU_CTRL_2_REGION_EN_MASK (BIT(PCIE_IATU_CTRL_2_REGION_EN_SHIFT))
#define PCIE_IATU_CTRL_2_FUNC_NUM_MATCH_EN_MASK (BIT(PCIE_IATU_CTRL_2_FUNC_NUM_MATCH_EN_SHIFT))
#define PCIE_IATU_CTRL_2_INVERTED_MODE_EN_MASK (BIT(PCIE_IATU_CTRL_2_INVERTED_MODE_EN_SHIFT))
#define PCIE_IATU_CTRL_2_RESPONSE_TYPE ((PCIE_IATU_CTRL_2_RESPONSE_TYPE_UNSUPPORTED) << PCIE_IATU_CTRL_2_RESPONSE_TYPE_SHIFT)

/* PCIe IATU definitions. */
#define PCIE_IATU_NUM_REGIONS (4)

/* PCIE Configuration space PCIe capability structure defines. */
/* PCIe Configuration space header command register defines. */
#define PCIE_DBI_TYPE0_STATUS_COMMAND_OFFSET (0x4)
#define PCIE_HDR_COMMAND_BUS_MASTER_ENABLE_SHIFT (2)

#define PCIE_HDR_COMMAND_BUS_MASTER_ENABLE_MASK \
	(BIT(PCIE_HDR_COMMAND_BUS_MASTER_ENABLE_SHIFT))

/* PCIE Configuration space MSI capability structure defines. */
#define PCIE_DBI_MSI_CAP_OFFSET (0x50)
/* MSI Enable, first bit of message control. */
#define PCIE_CFG_MSI_CAP_MSI_EN_MASK (BIT(0))

#define PCIE_REGS_OFFSET (0x800000)
#define PCIE_MSI_SII_DATA_VECTOR_SHIFT (8)

#define PCIE_MSI_SII_CTRL_MSI_REQ_SHIFT (0)
#define PCIE_MSI_SII_CTRL_MSI_ACKED_SHIFT (16)

#define PCIE_MSI_SII_CTRL_MSI_REQ_MASK (BIT(PCIE_MSI_SII_CTRL_MSI_REQ_SHIFT))
#define PCIE_MSI_SII_CTRL_MSI_ACKED_MASK \
	(BIT(PCIE_MSI_SII_CTRL_MSI_ACKED_SHIFT))

/* VPU Communications specific definitions. */
#define PCIE_COMMS_BOOT_SIZE (0x40)
#define BAR2_MAX_SIZE (32 * 1024 * 1024)

/* Call command(s) */
#define PCIE_BOOT_DMA_READ_ID (int)(0x444D4152)

/* IOCTL command(s) */
#define PCIE_BOOT_IATU_SETUP (int)(0x41545553)
#define PCIE_BOOT_BAR2_REMAP (int)(0x41545544)

struct __packed pcie_type0_header {
	u16 vendor_id;
	u16 device_id;
	u16 command;
	u16 status;
	u32 class_code_revision_id;
	u8 cache_line_size;
	u8 latency_timer;
	u8 header_type;
	u8 bist;
	u32 bar[6];
	u32 cardbus_cis_pointer;
	u16 subsystem_vendor_id;
	u16 subsystem_id;
	u32 expansion_rom_base;
	u8 capabilities_pointer;
	u8 reserved1;
	u16 reserved2;
	u32 reserved;
	u8 interrupt_line;
	u8 interrupt_pin;
	u8 min_gnt;
	u8 max_lat;
};

struct __packed pcie_msi_capability {
	u8 capability_id;
	u8 next_capability_pointer;
	u16 message_control;
	u32 message_address_lower;
	u32 message_address_upper;
	u16 message_data;
	u16 extended_message_data;
};

struct __packed pcie_dma_reg {
	u32 dma_ctrl_data_arb_prior;
	u32 reserved1;
	u32 dma_ctrl;
	u32 dma_write_engine_en;
	u32 dma_write_doorbell;
	u32 reserved2;
	u32 dma_write_channel_arb_weight_low;
	u32 dma_write_channel_arb_weight_high;
	u32 reserved3[3];
	u32 dma_read_engine_en;
	u32 dma_read_doorbell;
	u32 reserved4;
	u32 dma_read_channel_arb_weight_low;
	u32 dma_read_channel_arb_weight_high;
	u32 reserved5[3];
	u32 dma_write_int_status;
	u32 reserved6;
	u32 dma_write_int_mask;
	u32 dma_write_int_clear;
	u32 dma_write_err_status;
	u32 dma_write_done_imwr_low;
	u32 dma_write_done_imwr_high;
	u32 dma_write_abort_imwr_low;
	u32 dma_write_abort_imwr_high;
	u16 dma_write_ch_imwr_data[8];
	u32 reserved7[4];
	u32 dma_write_linked_list_err_en;
	u32 reserved8[3];
	u32 dma_read_int_status;
	u32 reserved9;
	u32 dma_read_int_mask;
	u32 dma_read_int_clear;
	u32 reserved10;
	u32 dma_read_err_status_low;
	u32 dma_read_err_status_high;
	u32 reserved11[2];
	u32 dma_read_linked_list_err_en;
	u32 reserved12;
	u32 dma_read_done_imwr_low;
	u32 dma_read_done_imwr_high;
	u32 dma_read_abort_imwr_low;
	u32 dma_read_abort_imwr_high;
	u16 dma_read_ch_imwr_data[8];
};

struct __packed pcie_dma_chan {
	u32 dma_ch_control1;
	u32 reserved1;
	u32 dma_transfer_size;
	u32 dma_sar_low;
	u32 dma_sar_high;
	u32 dma_dar_low;
	u32 dma_dar_high;
	u32 dma_llp_low;
	u32 dma_llp_high;
};

struct __packed pcie_iatu_inbound {
	u32 iatu_region_ctrl_1;
	u32 iatu_region_ctrl_2;
	u32 iatu_lwr_base_addr;
	u32 iatu_upper_base_addr;
	u32 iatu_limit_addr;
	u32 iatu_lwr_target_addr;
	u32 iatu_upper_target_addr;
};

struct __packed pcie_iatu_outbound {
        u32 iatu_region_ctrl_1;
        u32 iatu_region_ctrl_2;
        u32 iatu_lwr_base_addr;
        u32 iatu_upper_base_addr;
        u32 iatu_limit_addr;
        u32 iatu_lwr_target_addr;
        u32 iatu_upper_target_addr;
};

struct __packed pcie_regs {
	u32 subsystem_version;
	u32 cfg;
	u32 app_cntrl;
	u32 debug_aux;
	u32 debug_core;
	u32 sys_cntrl;
	u32 intr_enable;
	u32 intr_flags;
	u32 err_intr_enable;
	u32 err_intr_flags;
	u32 interrupt_enable;
	u32 interrupt_status;
	u32 misc_status;
	u32 msi_status;
	u32 msi_status_io;
	u32 msi_sii_data;
	u32 msi_sii_ctrl;
	u32 cfg_msi;
	u32 cfg_msi_data;
	u32 cfg_msi_addr0;
	u32 cfg_msi_addr1;
	u32 cfg_msi_mask;
	u32 cfg_msi_pending;
	u32 trgt_timeout;
	u32 radm_timeout;
	u32 vendor_msg_payload_0;
	u32 vendor_msg_payload_1;
	u32 vendor_msg_req_id;
	u32 ltr_msg_payload_0;
	u32 ltr_msg_payload_1;
	u32 ltr_msg_req_id;
	u32 sys_cfg_core;
	u32 sys_cfg_aux;
	u32 electr_mech;
	u32 cii_hdr_0;
	u32 cii_hdr_1;
	u32 cii_data;
	u32 cii_cntrl;
	u32 cii_override_data;
	u32 ltr_cntrl;
	u32 ltr_msg_latency;
	u32 cfg_ltr_max_latency;
	u32 app_ltr_latency;
	u32 sii_pm_state;
	u32 sii_pm_state_1;
	u32 mem_ctrl;
	u32 vmi_ctrl;
	u32 vmi_params_0;
	u32 vmi_params_1;
	u32 vmi_data_0;
	u32 vmi_data_1;
	u32 diag_ctrl;
	u32 diag_status_0;
	u32 diag_status_1;
	u32 diag_status_2;
	u32 diag_status_3;
	u32 diag_status_4;
	u32 diag_status_5;
	u32 diag_status_6;
	u32 diag_status_7;
	u32 diag_status_8;
	u32 diag_status_9;
	u32 diag_status_10;
	u32 diag_status_11;
	u32 diag_status_12;
	u32 diag_status_13;
	u32 diag_status_14;
	u32 diag_status_15;
	u32 diag_status_16;
	u32 diag_status_17;
	u32 diag_status_18;
	u32 diag_status_19;
	u32 diag_status_20;
	u32 diag_status_21;
	u32 cxpl_debug_info_0;
	u32 cxpl_debug_info_1;
	u32 cxpl_debug_info_ei;
	u32 mstr_rmisc_info_0;
	u32 slv_awmisc_info_0;
	u32 slv_awmisc_info_1;
	u32 slv_awmisc_info_2;
	u32 slv_awmisc_info_3;
	u32 slv_armisc_info_0;
	u32 slv_armisc_info_1;
	u32 pll_cntrl;
	u32 wake_csr;
	u32 ext_clk_cntrl;
	u32 lane_flip;
	u32 phy_cntl_stat;
	u32 ljpll_sta;
	u32 ljpll_cntrl_0;
	u32 ljpll_cntrl_1;
	u32 ljpll_cntrl_2;
	u32 ljpll_cntrl_3;
};

#endif /* __THB_PCIE_H__ */
