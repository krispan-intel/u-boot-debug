/*
 * Copyright (c) 2019-2020 Intel Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 * Original ATF 'platform_private.h' file, after removing unneeded information.
 * Added an 'error' type to the boot interfaces.
 */

#ifndef __PLATFORM_PRIVATE_H__
#define __PLATFORM_PRIVATE_H__

#include <asm/arch/boot/ma_boot.h>
#include <asm/arch/boot/ocs_hash.h>

/* Platform boot flow type. */
typedef enum {
	PLATFORM_NORMAL_FLOW = 0, /* Normal boot flow to OS. */
	PLATFORM_WAKE_FLOW, /* Wake flow to resume OS. */
	PLATFORM_RECOVERY_FLOW, /* HW Strap initiated FWU recovery flow of flash boot partition. */
	PLATFORM_FWU_FLOW /* User runtime initiated firmware update boot flow of flash boot partition. */
} platform_boot_flow_t;

/*
 * Platform partition type.
 *
 * Used to indicate which partition to load the FIP from.
 *
 * If FIP found in capsule partition then firmware enters firmware update flow.
 *
 * If fip found in boot partition this indicates which partition the system is booting from,
 * which partition to target for a firmware update/recovery, and which set
 * of NV_COUNTER eFuses are being used.
 *
 * Note for slave boots this value is always PLATFORM_BOOT_PARTITION_1
 * to indicate only the partition 1 eFuse NV_COUNTER fields are used.
 */
typedef enum {
	PLATFORM_BOOT_PARTITION_1 = 0, /* Load FIP from partition 1. */
	PLATFORM_BOOT_PARTITION_2,     /* Load FIP from partition 2. */
	PLATFORM_CAPSULE_PARTITION,    /* Load FIP Capsule partition. */
	PLATFORM_INACTIVE_PARTITION
} platform_partition_t;

/* Boot Interface enum */
typedef enum {
	MA_BOOT_INTF_EMMC = 0x0,
	MA_BOOT_INTF_PCIE = 0x1,
	MA_BOOT_INTF_SPI = 0x2,
	MA_BOOT_INTF_DEBUG = 0x3
} platform_boot_intf_t;

/* Firmware event item. */
typedef struct {
	const uint32_t id;
	const uint64_t param;
} platform_event_t;

/* BL1 context */
typedef struct {
	/*
	 * Size of this struct, set on reset.
	 * Other stages need to be aware that this size
	 * will be a constant for a particular silicon stepping.
	 */
	const unsigned int ctx_size;
	const int plat_error; /* Error from plat_error_handler. */
	const platform_event_t req_to_rom; /* Request to ROM. */
	const platform_event_t err_to_rt; /* Error to runtime. */
	const platform_boot_flow_t boot_flow;
	const ma_boot_type_t boot_type;
	const platform_partition_t boot_partition;
	const unsigned int upd_tfw_nv_ctr_val;
	const unsigned int cpu_freq;
	const int bl1_next_image_id;
	const uint8_t recovery_called;
	const platform_boot_intf_t boot_interface;
	const platform_boot_intf_t recovery_interface;
	const ma_boot_qual_t boot_qual;
	const size_t dev_erase_granularity_size;
	const size_t dev_partition_size;
	const size_t blk_dev_max_xfer_size;
	const u64 efuse_device_id; /* Only read for USBD and SPIS boot. */
	const u8 emmc_boot_params[80];
	const u8 spi_params[72];
	const u8 pcie_params[12];
	const uint8_t console_init_state;
} platform_bl1_ctx_t;

union platform_fuse_feature_excludes {
        struct {
                const uint64_t shave_0_3 : 1;
                const uint64_t shave_4_7 : 1;
                const uint64_t shave_8_11 : 1;
                const uint64_t shave_12_15 : 1;
                const uint64_t mipi_rx_1_0 : 1;
                const uint64_t mipi_rx_3_2 : 1;
                const uint64_t mipi_rx_5_4 : 1;
                const uint64_t mipi_rx_7_6 : 1;
                const uint64_t mipi_rx_9_8 : 1;
                const uint64_t slvds : 1;
                const uint64_t video_enc_hevc : 1;
                const uint64_t video_enc_h264 : 1;
                const uint64_t video_enc_dec_jpeg : 1;
                const uint64_t video_dec_hevc : 1;
                const uint64_t video_dec_h264 : 1;
                const uint64_t video_dec_h264_mvc : 1;
                const uint64_t video_dec_h264_scale_in_pp : 1;
                const uint64_t warp0 : 1;
                const uint64_t warp1 : 1;
                const uint64_t emmc : 1;
                const uint64_t sdio0 : 1;
                const uint64_t sdio1 : 1;
                const uint64_t ocs_aes : 1;
                const uint64_t ocs_sm4 : 1;
                const uint64_t ocs_sha2 : 1;
                const uint64_t ocs_sm3 : 1;
                const uint64_t ocs_sm2 : 1;
                const uint64_t dpu0 : 1;
                const uint64_t dpu1 : 1;
                const uint64_t dpu2 : 1;
                const uint64_t dpu3 : 1;
                const uint64_t dpu4 : 1;
                const uint64_t dpu5 : 1;
                const uint64_t dpu6 : 1;
                const uint64_t dpu7 : 1;
                const uint64_t dpu8 : 1;
                const uint64_t dpu9 : 1;
                const uint64_t dpu10 : 1;
                const uint64_t dpu11 : 1;
                const uint64_t dpu12 : 1;
                const uint64_t dpu13 : 1;
                const uint64_t dpu14 : 1;
                const uint64_t dpu15 : 1;
                const uint64_t dpu16 : 1;
                const uint64_t dpu17 : 1;
                const uint64_t dpu18 : 1;
                const uint64_t dpu19 : 1;
                const uint64_t a53_core1 : 1;
                const uint64_t a53_core2 : 1;
                const uint64_t a53_core3 : 1;
                const uint64_t matmul : 1;
                const uint64_t ddr_ch0 : 1;
                const uint64_t ddr_ch1 : 1;
                const uint64_t sw0 : 1;
                const uint64_t sw1 : 1;
                const uint64_t sw2 : 1;
                const uint64_t sw3 : 1;
                const uint64_t isp_tnf_dis : 1;
                const uint64_t isp_chroma_dis : 1;
                const uint64_t isp_hdr_dis : 1;
                const uint64_t stereo : 1;
                const uint64_t ultrasoc : 1;
                const uint64_t reserved : 2;
        } bit;
	const uint64_t value;
};

/*
 * Boot context struct with fields set by post ROM BL stages.
 */
typedef struct {
	/*
	 * Size of this struct. Set on BL2 entry.
	 */
	const unsigned int ctx_size;
	const int plat_error; /* Error from plat_error_handler. */
	const union platform_fuse_feature_excludes fuse_excludes; /* HW blocks excluded on silicon manufacture. */
	const platform_event_t err_to_rt;
	const uint32_t bl2_soc_rst_stat; /* SoC Reset before it was cleared by BL2. */
	const ocs_hash_alg_t tpm_hash_alg_type;
	const uint8_t tpm_secure_world_digest[SHA384_SIZE]; /* Hash of BL2 + BL31 + BL32. */
	const uint8_t tpm_normal_world_digest[SHA384_SIZE]; /* Hash of BL33. Size of max supported Hash */
	const uint8_t tpm_secure_world_bl2_digest[SHA384_SIZE]; /* Hash of BL2 calculated by BL1 */
	const uint32_t enc_fwu_key[AES_256_KEY_U32_SIZE]; /* FWU key pdat item. */
        const uint32_t enc_fwu_key_iv[AES_IV_U32_SIZE]; /* FWU key IV pdat item. */
	const uint8_t slice_en[4]; /*Slice info pdat item. */
        const uint8_t mem_id; /* Memory Config info pdat item*/
} platform_bl_ctx_t;

#endif /* __PLATFORM_PRIVATE_H__ */
