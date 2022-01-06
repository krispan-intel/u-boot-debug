// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2017 Linaro
 * Jorge Ramirez-Ortiz <jorge.ramirez-ortiz@linaro.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <image.h>
#ifdef CONFIG_DEBUG_UART
#include <debug_uart.h>
#endif /* CONFIG_DEBUG_UART */
#include <asm/arch/boot/platform_private.h>
#include <asm/arch/thb_shared_mem.h>
#include <asm/arch-thb/thb-boot-code.h>
#include <asm/arch/sip_svc.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <asm/armv8/mmu.h>
#include <hash.h>
#include <linux/libfdt.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <mmc.h>
#include <tpm-v2.h>
#include <tpm-common.h>
#include <version.h>
#include "thb-imr.h"
#include "thb_pad_cfg.h"
#include "thb_ddr_prof.h"
#include "tpm-eventlog.h"
#include <asm/arch/boot/thb-efuse.h>

#define GPIO_MICRON_FLASH_PULL_UP       20

/*
 * PCR USAGE
 * Index                 PCR Usage
 *   0     SRTM, BIOS, Host Platform Extensions, Embedded
 *         Option ROMs and PI Drivers
 *   1     Host Platform Configuration
 *   2     UEFI driver and application Code
 *   3     UEFI driver and application Configuration and Data
 *   4     UEFI Boot Manager Code (usually the MBR) and Boot Attempts
 *   5     Boot Manager Code Configuration and Data (for use by the
 *         Boot Manager Code) and GPT/Partition Table
 *   6     Host Platform Manufacturer Specific
 *   7     Secure Boot Policy
 *  8-15   Defined for use by the Static OS
 *   16    DebugSpecification TCG PC Client Platform Firmware Profile
 *   23    Application Support
 */

#define TPM_ROM_PCR_INDEX                       0
#define TPM_HPC_PCR_INDEX                       1
#define TPM_MBR_PCR_INDEX                       4
#define TPM_GPT_PCR_INDEX                       5
#define TPM_SECURE_BOOT_POLICY_PCR_INDEX        7
#define TPM_KERNEL_PCR_INDEX                    8

#define TPM_BL2_EVT_TYPE                EV_POST_CODE
#define TPM_FDT_EVT_TYPE                EV_TABLE_OF_DEVICES
#define TPM_BL33_EVT_TYPE               EV_COMPACT_HASH
#define TPM_KERNEL_EVT_TYPE             EV_COMPACT_HASH

#define SZ_8G                           0x200000000
#define BUFF_LEN			6
#define ULT_BUFF_LEN			20
#define DRAM_SZ_LEN			10
#define MRC_VER_LEN			5

#define SVN_START_ADDR			0xA0
#define SVN_END_ADDR			0xA3

const char version_string[] = U_BOOT_VERSION_STRING CC_VERSION_STRING;

extern int get_tpm(struct udevice **devp);
static int get_bl_ctx(platform_bl_ctx_t *bl_ctx);

phys_size_t get_effective_memsize(void);

u8 board_type_crb2 __attribute__ ((section(".data")));
u8 board_type_hddl __attribute__ ((section(".data")));
u8 board_id __attribute__ ((section(".data")));
u64 ult_info __attribute__ ((section(".data")));

/* Mapping of TBH Prime Slices and Memory Cfg */
u8 slice_mem_map[SLICE_INDEX][MEM_INDEX] __attribute__ ((section(".data")));
u8 slice[4] __attribute__ ((section(".data")));
u8 thb_full __attribute__ ((section(".data")));
u8 mem_id __attribute__ ((section(".data")));
unsigned long long total_mem __attribute__ ((section(".data")));
char fdt_mrc_version[MRC_VER_LEN]  __attribute__ ((section(".data")));
char fdt_dram_sz[DRAM_SZ_LEN] __attribute__ ((section(".data")));

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region thb_mem_map[] = {
	{       /* Memory mapped registers: 0x8000_0000 - 0x8880_0000 */
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = 0x8880000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) | PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN |
			 PTE_BLOCK_UXN
	},
	{       /* Memory mapped registers: PCIe AXI base */
		.virt = 0x2000000000UL,
		.phys = 0x2000000000UL,
		.size = 0x1000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) | PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN |
			 PTE_BLOCK_UXN
	},
	{       /*
		 * DRAM: set up page tables for:
		 * Start of usable DDR: DDR_BASE + SECURE_DDR_SIZE
		 * Across usable DDR size: UBOOT_SDRAM_SIZE
		 * update the size based on dynamic detected RAM size in next release SK:*/
		.virt = (u64)CONFIG_SYS_SDRAM_BASE,
		.phys = (u64)CONFIG_SYS_SDRAM_BASE,
		.size = (u64)UBOOT_SDRAM_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_OUTER_SHARE
	},
	{
		/* Also map the memory shared with secured world. */
		.virt = (u64)SHARED_DDR_BASE,
		.phys = (u64)SHARED_DDR_BASE,
		.size = (u64)SHARED_DDR_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_OUTER_SHARE,
	},
	{
		0,
	}
};

struct mm_region *mem_map = thb_mem_map;

struct fdt_string_prop {
	const char *prop;
	const char *value;
};

#define NUM_FIRMWARE_BIOS_PROPERTIES (5)
static const struct fdt_string_prop
	firmware_bios_properties[NUM_FIRMWARE_BIOS_PROPERTIES] = {
	{ "system-manufacturer", CONFIG_SYS_VENDOR },
	{ "system-product-name", CONFIG_SYS_BOARD },
	{ "bios-vendor", "Intel" },
	{ "bios-version", U_BOOT_VERSION },
	{ "bios-release-date", U_BOOT_DATE " " U_BOOT_TIME },
};

#define NUM_FIRMWARE_ATF_PROPERTIES (3)
static const struct fdt_string_prop
	firmware_atf_properties[NUM_FIRMWARE_ATF_PROPERTIES] = {
	{ "arm-trusted-firmware-version", "v1.5" },
	{ "arm-trusted-firmware-vendor", "Intel" },
	{ "arm-trusted-firmware-release-date",
	  U_BOOT_DATE " " U_BOOT_TIME },
};

static platform_boot_intf_t boot_interface = MA_BOOT_INTF_EMMC;
static uint8_t boot_mode __attribute__ ((section(".data")));
static uint8_t measured_boot __attribute__ ((section(".data")));
static uint8_t soc_rev __attribute__ ((section(".data")));

static void fdt_dram_mem(uint64_t size)
{
	unsigned long m = 0, n;
	uint64_t f;
	static const char names[] = {'E', 'P', 'T', 'G', 'M', 'K'};
	unsigned long d = 10 * ARRAY_SIZE(names);
	char c = 0;
	unsigned int i;
	char size_buf[BUFF_LEN];
	char dec_size_buf[BUFF_LEN];

	for (i = 0; i < ARRAY_SIZE(names); i++, d -= 10) {
		if (size >> d) {
			c = names[i];
			break;
		}
	}

	if (!c) {
		snprintf(fdt_dram_sz, DRAM_SZ_LEN, "%llu Bytes", size);
		return;
	}

	n = size >> d;
	f = size & ((1ULL << d) - 1);

	/* If there's a remainder, deal with it */
	if (f) {
		m = (10ULL * f + (1ULL << (d - 1))) >> d;

		if (m >= 10) {
			m -= 10;
			n += 1;
		}
	}

	snprintf(size_buf, BUFF_LEN, "%lu", n);

	if (m) {
		snprintf(dec_size_buf, BUFF_LEN, ".%ld", m);
		snprintf(fdt_dram_sz, DRAM_SZ_LEN, "%s%s %cB", size_buf, dec_size_buf, c);
		return;
	}

	snprintf(fdt_dram_sz, DRAM_SZ_LEN, "%s %cB", size_buf, c);

	return;
}

static int fdt_create_node_and_populate(void *fdt, int nodeoffset,
					const char *nodestring, u32 array_size,
					const struct fdt_string_prop *prop_arr)
{
	u32 i;
	int ret;
	int newnodeoffset;

	/* Create "nodestring" @ nodeoffset, if it didn't exist. */
	newnodeoffset = fdt_find_or_add_subnode(fdt, nodeoffset, nodestring);
	if (newnodeoffset < 0) {
		printf("Couldn't find or create node \"%s\".\n", nodestring);
		return newnodeoffset;
	}

	/* Add our information. */
	for (i = 0; i < array_size; i++) {
		ret = fdt_setprop_string(fdt, newnodeoffset, prop_arr[i].prop,
					 prop_arr[i].value);
		if (ret < 0) {
			printf("Couldn't add property: %s\n", prop_arr[i].prop);
			return ret;
		}
	}

	return 0;
}


/*
 * THB EVT2 board has smbus@80480000, but  not previous board varints
 * SMBUS node will be disabled in DTS file for all board varints, it will
 * be enabled for evt2 in fdt_thb_smbus_fixup
 *
 * SMBUS will have different address for THB and THB'. Slave addresses are
 * updated here.
 *	THB_FULL:  slave address 0x4000005a
 *	THB PRIME: slave address 0x4000006a
 */

static int fdt_thb_smbus_fixup(void *fdt)
{
	int smbus_dev_off = 0, node = 0;
	int ret;

	smbus_dev_off =  fdt_path_offset(fdt, "/soc/smbus@80480000");
	if (smbus_dev_off < 0) {
		log_err("Failed to find smbus node.\n");
		return smbus_dev_off;
	}
	/* BOARD_TYPE_HDDLF2 is for EVT2 */
	if (BOARD_TYPE_HDDLF2 == board_id) {
		ret = fdt_setprop_string(fdt, smbus_dev_off, "status", "okay");

		if (ret) {
			log_err("Failed to update id in smbus_device node status\n");
			return ret;
		}

		fdt_for_each_subnode(node, fdt, smbus_dev_off) {
			int len;
			const char *node_name;
			unsigned int slave_addr = 0;

			node_name = fdt_get_name(fdt, node, &len);
			if (len < 0) {
				log_err("unable to get node name.\n");
				return len;
			}
			if (!strcmp(node_name, "slave_device")) {
				/* THB_FULL:  slave address 0x4000005a
				 * THB PRIME: slave address 0x4000006a
				 * thb_full 1 for THB full and 0 for THB prime
				 */
				if (thb_full)
					slave_addr = 0x4000005a;
				else
					slave_addr = 0x4000006a;
				ret = fdt_setprop_u32(fdt, node, "reg", slave_addr);

				if (ret) {
					log_err("Failed to update id in smbus slave address\n");
					return ret;
				}
			}
		}
	}
	return 0;
}

/*
 * I2C0 is used as Master in evt2 board.
 *
 * I2C0 slave device node will be 'disabled' in dts file, however it will be
 * "okay" for evt2 in fdt_thb_i2c0_fixup
 */

static int fdt_thb_i2c0_fixup(void *fdt)
{
	int i2c_dev_off = 0, node = 0;
	int ret;

	i2c_dev_off =  fdt_path_offset(fdt, "/soc/i2c@80490000");
	if (i2c_dev_off < 0) {
		log_err("Failed to find i2c0 node.\n");
		return i2c_dev_off;
	}

	 /* BOARD_TYPE_HDDLF2 is for EVT2 */
	if (BOARD_TYPE_HDDLF2 == board_id) {

		fdt_for_each_subnode(node, fdt, i2c_dev_off) {
			int len;
			const char *node_name;

			node_name = fdt_get_name(fdt, node, &len);
			if (len < 0) {
				log_err("unable to get node name.\n");
				return len;
			}
			if (!strcmp(node_name, "adc081c@51")) {
					ret = fdt_setprop_string(fdt, node, "status", "disabled");
			}
			if (thb_full) {
				if (!strcmp(node_name, "pmic@47")) {
					ret = fdt_setprop_string(fdt, node, "status", "okay");
			}
				if (!strcmp(node_name, "pmic@57")) {
					ret = fdt_setprop_string(fdt, node, "status", "okay");
			}
			}
			else {
				if (!strcmp(node_name, "pmic@57")) {
					ret = fdt_setprop_string(fdt, node, "status", "okay");
			}
			}
		}
	}

	return 0;
}

/*
 * I2C1 is used as Master in evt2 board, however it is used as slave in
 * other board variants.
 *
 * I2C1 slave device node will be 'okay' in dts file, however it will be
 * disabled for evt2 in fdt_thb_i2c_fixup
 */

static int fdt_thb_i2c_fixup(void *fdt)
{
	int i2c_dev_off = 0, node = 0;
	int ret;

	i2c_dev_off =  fdt_path_offset(fdt, "/soc/i2c@804a0000");
	if (i2c_dev_off < 0) {
		log_err("Failed to find i2c1 node.\n");
		return i2c_dev_off;
	}

	 /* BOARD_TYPE_HDDLF2 is for EVT2 */
	if (BOARD_TYPE_HDDLF2 == board_id) {

		fdt_for_each_subnode(node, fdt, i2c_dev_off) {
			int len;
			const char *node_name;

			node_name = fdt_get_name(fdt, node, &len);
			if (len < 0) {
				log_err("unable to get node name.\n");
				return len;
			}
			if (!strcmp(node_name, "slave_device@5a")) {
				ret = fdt_setprop_string(fdt, node, "reg", "0x00");
				ret = fdt_setprop_string(fdt, node, "status", "disabled");
			}
			if (!strcmp(node_name, "fruram@50")) {
                                ret = fdt_setprop_string(fdt, node, "status", "okay");
			}
			if (!strcmp(node_name, "tmp112@48")) {
                                ret = fdt_setprop_string(fdt, node, "status", "okay");
			}
			if (!strcmp(node_name, "adc081c@18")) {
			       ret = fdt_setprop_string(fdt, node, "status", "okay");
			}
			if (!strcmp(node_name, "pcal9575@20")) {
	                       ret = fdt_setprop_string(fdt, node, "status", "okay");
	                }
		}
	}

	return 0;
}

/*
 * I2C3 is used as Master in evt2 board.
 *
 * I2C3 slave device node will be 'disabled' in dts file, however it will be
 * "okay" for evt2 in fdt_thb_i2c3_fixup
 */

static int fdt_thb_i2c3_fixup(void *fdt)
{
	int i2c_dev_off = 0, node = 0;
	int ret;

	i2c_dev_off =  fdt_path_offset(fdt, "/soc/i2c@804c0000");
	if (i2c_dev_off < 0) {
		log_err("Failed to find i2c3 node.\n");
		return i2c_dev_off;
	}

	 /* BOARD_TYPE_HDDLF2 is for EVT2 */
	if (BOARD_TYPE_HDDLF2 == board_id) {

		fdt_for_each_subnode(node, fdt, i2c_dev_off) {
			int len;
			const char *node_name;

			node_name = fdt_get_name(fdt, node, &len);
			if (len < 0) {
				log_err("unable to get node name.\n");
				return len;
			}
			if (!strcmp(node_name, "pcisw@42")) {
				ret = fdt_setprop_string(fdt, node, "status", "okay");
			}
		}
	}
	return 0;
}

static int fdt_thb_model_fixup(void *fdt)
{
	int model_dev_off = 0, node = 0;
	int ret;

	model_dev_off = fdt_path_offset(fdt, "/");
	if(model_dev_off < 0) {
		log_err("Failed to find model property node.\n");
		return model_dev_off;
	}

	if (soc_rev == 0x00)
		ret = fdt_setprop_string(fdt, model_dev_off, "model", "Intel Thunder Bay A0");
	else if (soc_rev == 0x01)
		ret = fdt_setprop_string(fdt, model_dev_off, "model", "Intel Thunder Bay A1");
	else
		ret = fdt_setprop_string(fdt, model_dev_off, "model", "Intel Thunder Bay Unknown");

	return 0;
}

static int fdt_thb_evt2_hddl_fixup(void *fdt, int hddl_dev_off)
{
	int node = 0, ret;

	fdt_for_each_subnode(node, fdt, hddl_dev_off) {
		int len;
		const char *node_name;
		int bus = 1;

		node_name = fdt_get_name(fdt, node, &len);
		if (len < 0) {
			log_err("unable to get node name.\n");
			return len;
		}

		if (!strcmp(node_name, "kmb_xlink_tj@5a"))
			bus = 4;

		if (!strcmp(node_name, "tmp112@48")) {
			ret = fdt_setprop_u32(fdt, node, "reg", 0x48);
			if (ret) {
				log_err("Failed to update client addr for tmp112\n");
				return ret;
			}
		}
		if (!strcmp(node_name, "adc081c@51"))
			ret = fdt_setprop_string(fdt, node, "status", "disabled");

		if (!strcmp(node_name, "ads7142@18"))
			ret = fdt_setprop_string(fdt, node, "status", "okay");

		ret = fdt_setprop_u32(fdt, node, "bus",
				      bus);
		if (ret) {
			log_err("Failed to update board id in hddl_device node\n");
			return ret;
		}
	}

	return 0;
}

static int fdt_thb_hddl_dev_fixup(void *fdt)
{
	int hddl_dev_off = 0, tsens_off = 0, node = 0;
	int ret;
	platform_bl_ctx_t plat_bl_ctx;

	hddl_dev_off =  fdt_path_offset(fdt, "/soc/hddl_device");
	if (hddl_dev_off < 0) {
		log_err("Failed to find hddl_device node.\n");
		return hddl_dev_off;
	}
	/* Get BL context structure */
	ret = get_bl_ctx(&plat_bl_ctx);
	if (ret) {
		panic("Failed to retrieve bl ctx, slice and memory selection failed\n");
	}
	ret = fdt_setprop_u32(fdt, hddl_dev_off, "id",
        	              plat_bl_ctx.soc_id);
	if (ret) {
        	log_err("Failed to update id in hddl_device node\n");
	        return ret;
	}
	ret = fdt_setprop_u32(fdt, hddl_dev_off, "board_id",
			      plat_bl_ctx.io_exp_addr_bits);
	if (ret) {
		log_err("Failed to update board id in hddl_device node\n");
		return ret;
	}

	ret = fdt_setprop_u32(fdt, hddl_dev_off, "board_type",
        	              plat_bl_ctx.board_id);
	if (ret) {
        	log_err("Failed to update board id in hddl_device node\n");
	        return ret;
	}

	/* BOARD_TYPE_HDDLF2 is for EVT2 */
	if (board_id == BOARD_TYPE_HDDLF2) {
		ret = fdt_thb_evt2_hddl_fixup(fdt, hddl_dev_off);
		if (ret) {
			log_err("Failed to update bus num for evt2 sensors\n");
			return ret;
		}
	}

	tsens_off =  fdt_path_offset(fdt, "/soc/tsens");
	if (tsens_off < 0) {
		log_err("Failed to find tsens node.\n");
		return tsens_off;
	}
	ret = fdt_setprop_u32(fdt, tsens_off, "board_type",
        	              plat_bl_ctx.board_id);
	if (ret) {
        	log_err("Failed to update board id in tsens node\n");
	        return ret;
	}

	fdt_for_each_subnode(node, fdt, tsens_off)
	{
		int len;
		const char *node_name;

		node_name = fdt_get_name(fdt, node, &len);
		if (len < 0) {
			log_err("unable to get node name.\n");
			return len;
		}
		if (!strcmp(node_name, "cpu_s")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.cpu_s_calib_offset);
		}
		else if (!strcmp(node_name, "cpu_n")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.cpu_n_calib_offset);
		}
		else if (!strcmp(node_name, "vpu0")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.vpu0_calib_offset);
		}
		else if (!strcmp(node_name, "media0")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.media0_calib_offset);
		}
		else if (!strcmp(node_name, "vddr0")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.vddr0_calib_offset);
		}
		else if (!strcmp(node_name, "vpu1")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.vpu1_calib_offset);
		}
		else if (!strcmp(node_name, "media1")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.media1_calib_offset);
		}
		else if (!strcmp(node_name, "vddr1")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.vddr1_calib_offset);
		}
		else if (!strcmp(node_name, "vpu2")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.vpu2_calib_offset);
		}
		else if (!strcmp(node_name, "media2")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.media2_calib_offset);
		}
		else if (!strcmp(node_name, "vddr2")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.vddr2_calib_offset);
		}
		else if (!strcmp(node_name, "vpu3")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.vpu3_calib_offset);
		}
		else if (!strcmp(node_name, "media3")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.media3_calib_offset);
		}
		else if (!strcmp(node_name, "vddr3")) {
			ret = fdt_setprop_u32(fdt, node, "calib_off",
				plat_bl_ctx.dts_calibs.vddr3_calib_offset);
		}
		if (ret) {
			log_err("Failed to update calib for %s\n", node_name);
			return ret;
		}
	}
	return 0;
}

static int fdt_thb_boot_info(void *fdt)
{
	int boot_info_off = 0;
	int ret = 0;
	char *fdt_boot_intf = NULL;
	char *fdt_soc_rev = NULL;
	char *fdt_boot_mode = NULL;
	char *fdt_measured_boot = NULL;
	char *fdt_soc_cfg = NULL;
	u8 mem_rank = 0;
	u8 mem_density = 0;
	char fdt_board_id[BUFF_LEN];
	char fdt_mem_rank[BUFF_LEN];
	char fdt_mem_density[BUFF_LEN];
	char fdt_ult_info[ULT_BUFF_LEN];

	boot_info_off =  fdt_path_offset(fdt, "/boot_info");
	if (boot_info_off < 0) {
		log_err("Failed to find boot_info node.\n");
		boot_info_off = fdt_add_subnode(fdt, 0, "boot_info");
		if (boot_info_off < 0) {
			log_err("Failed to create boot_info node.\n");
			return ret;
		}

		boot_info_off =  fdt_path_offset(fdt, "/boot_info");
	}

	if (boot_interface == MA_BOOT_INTF_EMMC)
		fdt_boot_intf = "eMMC";
	else if (boot_interface == MA_BOOT_INTF_PCIE)
		fdt_boot_intf = "PCIe";

	if (!boot_mode)
		fdt_boot_mode = "Open boot";
	else
		fdt_boot_mode = "Secure boot";

	if (measured_boot)
		fdt_measured_boot = "Measured boot enabled";
	else
		fdt_measured_boot = "Measured boot disabled";

	if (soc_rev == 0x00)
		fdt_soc_rev = "THB(A0)";
	else if (soc_rev == 0x01)
		fdt_soc_rev = "THB(A1)";

	if (thb_full)
		fdt_soc_cfg = "All slices enabled";
	else if (slice[0] & slice[2])
		fdt_soc_cfg = "slice_0_2 enabled";
	else if (slice[0] & slice[3])
		fdt_soc_cfg = "slice_0_3 enabled";
	else if (slice[1] & slice[2])
		fdt_soc_cfg = "slice_1_2 enabled";
	else if (slice[1] & slice[3])
		fdt_soc_cfg = "slice_1_3 enabled";

	mem_rank = mem_id &0x01;
	mem_density = (mem_id >> 1) & 0x3;

	snprintf(fdt_board_id, BUFF_LEN, "0x%02x", board_id);
	snprintf(fdt_mem_rank, BUFF_LEN, "0x%02x", mem_rank);
	snprintf(fdt_mem_density, BUFF_LEN, "0x%02x", mem_density);
	snprintf(fdt_ult_info, ULT_BUFF_LEN, "0x%llx", ult_info);

	fdt_dram_mem(total_mem);

	ret = fdt_setprop_string(fdt, boot_info_off, "fw_date_time", U_BOOT_VERSION_STRING);
	if (ret) {
		log_err("Failed to update FW data and time stamp in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "board_id", fdt_board_id);
	if (ret) {
		log_err("Failed to update board id in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "boot_intf", fdt_boot_intf);
	if (ret) {
		log_err("Failed to update boot interface in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "soc_rev", fdt_soc_rev);
	if (ret) {
		log_err("Failed to update soc revision in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "boot_mode", fdt_boot_mode);
	if (ret) {
		log_err("Failed to update boot mode in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "measured_boot", fdt_measured_boot);
        if (ret) {
                log_err("Failed to update measured boot in boot_info node\n");
                return ret;
        }

	ret = fdt_setprop_string(fdt, boot_info_off, "mrc_ver", fdt_mrc_version);
	if (ret) {
		log_err("Failed to update mrc version in boot_mode node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "mem_rank", fdt_mem_rank);
	if (ret) {
		log_err("Failed to update memory rank in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "mem_density", fdt_mem_density);
	if (ret) {
		log_err("Failed to update memory density in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "total_mem", fdt_dram_sz);
	if (ret) {
		log_err("Failed to update total memory in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "soc_cfg", fdt_soc_cfg);
	if (ret) {
		log_err("Failed to update soc config in boot_info node\n");
		return ret;
	}

	ret = fdt_setprop_string(fdt, boot_info_off, "ult_val", fdt_ult_info);
	if (ret) {
		log_err("Failed to update ult value in boot_info node\n");
		return ret;
	}

	return 0;
}

static int fdt_thb_ip_dis(void *fdt)
{
	int eth0_dev_off = 0, eth1_dev_off = 0, pcie_rc_dev_off = 0, node = 0;
	int ret;

	if ((board_id == BOARD_TYPE_HDDLF1) || (board_id == BOARD_TYPE_HDDLF2) || board_id == BOARD_TYPE_CRB2F1) {
		eth0_dev_off =  fdt_path_offset(fdt, "/soc/ethernet@804F0000");
		if (eth0_dev_off < 0) {
			log_err("Failed to find ethernet0 node.\n");
			return eth0_dev_off;
		}
		ret = fdt_setprop_string(fdt, eth0_dev_off, "status", "disabled");
		if (ret) {
			log_err("Failed to disable ethernet0 node\n");
			return ret;
		}

		eth1_dev_off =  fdt_path_offset(fdt, "/soc/ethernet@80500000");
		if (eth1_dev_off < 0) {
			log_err("Failed to find ethernet1 node.\n");
			return eth1_dev_off;
		}
		ret = fdt_setprop_string(fdt, eth1_dev_off, "status", "disabled");
		if (ret) {
			log_err("Failed to disable ethernet1 node\n");
			return ret;
		}

		pcie_rc_dev_off =  fdt_path_offset(fdt, "/soc/pcie@82400000");
		if (pcie_rc_dev_off < 0) {
			log_err("Failed to find pcie rp node.\n");
			return pcie_rc_dev_off;
		}
		ret = fdt_setprop_string(fdt, pcie_rc_dev_off, "status", "disabled");
		if (ret) {
			log_err("Failed to disable pcie rp node\n");
			return ret;
		}
	}

	return 0;
}

/*
 * Add some board-specific data to the FDT before booting the
 * OS.
 */
int ft_board_setup(void *fdt, struct bd_info *bd)
{
	int offset = 0;
	int ret = 0;

	/* Add information about firmware to DT. */

	/* Create "/firmware" if it didn't exist. */
	offset = fdt_find_or_add_subnode(fdt, 0, "firmware");
	if (offset < 0) {
		printf("Couldn't find or create /firmware.\n");
		return offset;
	}

	/* Create and populate "/firmware/bios" */
	ret = fdt_create_node_and_populate(fdt, offset, "bios",
					   NUM_FIRMWARE_BIOS_PROPERTIES,
					   firmware_bios_properties);
	if (ret < 0) {
		return ret;
	}

	/* Create and populate "/firmware/arm-trusted-firmware" */
	ret = fdt_create_node_and_populate(fdt, offset, "arm-trusted-firmware",
					   NUM_FIRMWARE_ATF_PROPERTIES,
					   firmware_atf_properties);
	if (ret < 0) {
		return ret;
	}
	ret = fdt_thb_hddl_dev_fixup(fdt);
	if (ret < 0) {
		return ret;
	}

	ret = fdt_thb_smbus_fixup(fdt);
	if (ret < 0) {
		log_err("Failed to update smbus property\n");
	}

	ret = fdt_thb_i2c0_fixup(fdt);
        if (ret < 0) {
		log_err("Failed to update i2c-0 property\n");
	}

	ret = fdt_thb_i2c_fixup(fdt);
	if (ret < 0) {
		log_err("Failed to update i2c-1 property\n");
	}

	ret = fdt_thb_boot_info(fdt);
	if (ret < 0) {
		return ret;
	}

	ret = fdt_thb_i2c3_fixup(fdt);
        if (ret < 0) {
		log_err("Failed to update i2c-3 property\n");
        }

	ret = fdt_thb_model_fixup(fdt);
	if (ret < 0) {
		log_err("Failed to update thb-model property\n");
	}

	ret = fdt_thb_ip_dis(fdt);
	if (ret < 0) {
                log_err("Failed to update eth0,eth1,pcie_rc property\n");
        }

	return 0;
}

/*
 * We want that the device tree appended to the U-Boot image,
 */
static void setup_fdt(void)
{
	/*
	 * Make sure to clear fdt_addr_r, this will trigger
	 * sysboot to go and load fdtfile into fdt_addr_r.
	 */
	if (env_get("fdt_addr_r")) {
		printf("Clearing fdt_addr_r.\n");
		env_set("fdt_addr_r", NULL);
	}

	/* Set FDT as the one at the end of U-Boot. */
	printf("Setting fdt_addr to 0x%p.\n", gd->fdt_blob);
	env_set_hex("fdt_addr", (unsigned long)gd->fdt_blob);
}

int board_boot_fail(unsigned int code)
{
	printf("Boot failed due to 0x%x - halting boot.\n", code);
	hang();
}

/*
 * Retrieve an area that can be shared with the secure world for
 * communication.
 */
void __iomem *get_secure_shmem_ptr(size_t min_size)
{
	int offset = 0;
	fdt_addr_t shmem_addr;
	fdt_size_t shmem_size;

	offset = fdt_subnode_offset(gd->fdt_blob, 0, "general_sec_shmem");
	if (offset < 0) {
		printf("Couldn't find general_sec_shmem.\n");
		return NULL;
	}

	offset = fdtdec_lookup_phandle(gd->fdt_blob, offset, "shmem");
	if (offset < 0) {
		printf("No shared memory defined for this device.\n");
		return NULL;
	}

	shmem_addr = fdtdec_get_addr_size_auto_parent(gd->fdt_blob,
						      0, offset, "reg",
						      0, &shmem_size, true);
	if (shmem_addr == FDT_ADDR_T_NONE) {
		printf("No memory found.\n");
		return NULL;
	}

	if (min_size > shmem_size) {
		return NULL;
	}

	return (void __iomem *)shmem_addr;
}

static int measure_boot_kernel_fdt(ocs_hash_alg_t hash_alg)
{
	struct udevice *dev;
	/*
	 * Note: The 'images' variable is declared in image.h and defined in
	 * bootm.c.
	 */
	image_info_t *os = &images.os;
	struct hash_algo *thb_hash_algo;

	/* SHA384_SIZE is maxmium size supported for measure boot.
	 * SHA384 is not supported in dTPM Infineon chipset.
	 * HSD : https://hsdes.intel.com/appstore/article/#/1508191880
	 */
	u8 kernel_hash[SHA384_SIZE];
	u8 fdt_hash[SHA384_SIZE];
	int rc = 0;

	debug("%s: image load at %lx, size: %lx\n", __func__,
	      os->load, os->image_len);

	rc = get_tpm(&dev);
	if (rc) {
		return rc;
	}

	if (hash_alg == OCS_HASH_SHA256) {
		rc = hash_lookup_algo("sha256", &thb_hash_algo);
	}
	if (hash_alg == OCS_HASH_SHA384) {
		rc = hash_lookup_algo("sha384", &thb_hash_algo);
	}
	if (rc) {
		printf("Hash algorithm look up failed\n");
		return -EINVAL;
	}

	debug("%s: %s selected for measure boot hash alg\n", __func__,
	      thb_hash_algo->name);

	thb_hash_algo->hash_func_ws((void *)os->load, os->image_len,
				    kernel_hash, thb_hash_algo->chunk_size);

	if (hash_alg == OCS_HASH_SHA256) {
		printf("Measured boot SHA256: Writing to THB_TPM_KERNEL_PCR_INDEX ...\n");
		rc = tpm_extend_pcr_and_log_event(dev, TPM_KERNEL_PCR_INDEX,
						  TPM2_ALG_SHA256, kernel_hash,
						  TPM_KERNEL_EVT_TYPE,
						  sizeof("THB_SOC_KERNEL"),
						  "THB_SOC_KERNEL");
		if (rc) {
			printf("tpm_extend_pcr_and_log_event failed ret %d\n", rc);
			return -EINVAL;
		}
	}
	if (hash_alg == OCS_HASH_SHA384) {
		printf("Measured boot SHA384: Writing to THB_TPM_KERNEL_PCR_INDEX ...\n");
		rc = tpm_extend_pcr_and_log_event(dev, TPM_KERNEL_PCR_INDEX,
						  TPM2_ALG_SHA384, kernel_hash,
						  TPM_KERNEL_EVT_TYPE,
						  sizeof("THB_SOC_KERNEL"),
						  "THB_SOC_KERNEL");
		if (rc) {
			printf("tpm_extend_pcr_and_log_event failed ret %d\n", rc);
			return -EINVAL;
		}
	}
	if (rc) {
		printf("%s: tpm2_pcr_extend failed ret %d\n", __func__, rc);
		return -EINVAL;
	}
	printf("Measured boot : Writing to THB_TPM_KERNEL_PCR_INDEX ...SUCCESS\n");

	thb_hash_algo->hash_func_ws((void *)images.ft_addr, images.ft_len,
				    fdt_hash, thb_hash_algo->chunk_size);

	if (hash_alg == OCS_HASH_SHA256) {
		printf("Measured boot SHA256: Writing to THB_TPM_FDT_PCR_INDEX...\n");
		rc = tpm_extend_pcr_and_log_event(dev, TPM_HPC_PCR_INDEX,
						  TPM2_ALG_SHA256, fdt_hash,
						  TPM_FDT_EVT_TYPE,
						  sizeof("THB_SOC_FDT"),
						  "THB_SOC_FDT");
		if (rc) {
			printf("tpm_extend_pcr_and_log_event failed ret %d\n", rc);
			return -EINVAL;
		}
	}
	if (hash_alg == OCS_HASH_SHA384) {
		printf("Measured boot SHA384: Writing to THB_TPM_FDT_PCR_INDEX...\n");
		rc = tpm_extend_pcr_and_log_event(dev, TPM_HPC_PCR_INDEX,
						  TPM2_ALG_SHA384, fdt_hash,
						  TPM_FDT_EVT_TYPE,
						  sizeof("THB_SOC_FDT"),
						  "THB_SOC_FDT");
		if (rc) {
			printf("tpm_extend_pcr_and_log_event failed ret %d\n", rc);
			return -EINVAL;
		}
	}
	if (rc) {
		printf("%s: tpm2_pcr_extend failed ret %d\n", __func__, rc);
		return -EINVAL;
	}
	printf("Measured boot : Writing to THB_TPM_FDT_PCR_INDEX...SUCCESS\n");

	return rc;
}

/**
 * get_bl_ctx() - Get BL context.
 * @bl_ctx: Where to store the retrieved context.
 *
 * Get the BL context (which includes various information, such as the platform
 * data and BL2+BL31+BL32+BL33 hash) from the runtime monitor.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int get_bl_ctx(platform_bl_ctx_t *bl_ctx)
{
	platform_bl_ctx_t *shmem_bl_ctx = NULL;
	int rc = 0;

	if (!bl_ctx) {
		return -EINVAL;
	}

	shmem_bl_ctx = get_secure_shmem_ptr(sizeof(*shmem_bl_ctx));
	if (!shmem_bl_ctx) {
		printf("No shared mem for communicating with secure world.\n");
		return -ENOMEM;
	}

	/* Set to zero */
	memset(shmem_bl_ctx, 0x0, sizeof(*shmem_bl_ctx));

	/* Request the information from the secure world */
	rc = sip_svc_get_bl_ctx((uintptr_t)shmem_bl_ctx, sizeof(*shmem_bl_ctx));
	if (rc) {
		printf("Failed to retrieve boot context.\n");
		return -EINVAL;
	}

	if (sizeof(*shmem_bl_ctx) != shmem_bl_ctx->ctx_size) {
		printf("Context size inconsistent.\n");
		return -EPROTO;
	}

	memcpy(bl_ctx, shmem_bl_ctx, sizeof(*bl_ctx));

	return 0;
}

/*
 * Query the platform BL context, which includes various information,
 * retrieve firmware hash algo used in ROM and the digests of
 * secure world and normal world.
 */
static int measure_boot_bl2(ocs_hash_alg_t *hash_alg)
{
	platform_bl_ctx_t *bl_ctx = NULL;
	int rc = 0;
	u32 tpm_hash_alg;
	struct udevice *dev;

	rc = get_tpm(&dev);
	if (rc) {
		return rc;
	}

	bl_ctx = get_secure_shmem_ptr(sizeof(*bl_ctx));
	if (!bl_ctx) {
		printf("No shared mem for communicating with secure world.\n");
		return -EINVAL;
	}

	/* Set to zero */
	memset(bl_ctx, 0x0, sizeof(*bl_ctx));

	/* Request the information from the secure world */
	rc = sip_svc_get_bl_ctx((uintptr_t)bl_ctx, sizeof(*bl_ctx));
	if (rc) {
		printf("Failed to retrieve boot context.\n");
		return -EINVAL;
	}

	if (sizeof(*bl_ctx) != bl_ctx->ctx_size) {
		printf("Context size inconsistent.\n");
		return -EINVAL;
	}

	struct plat_tpm_measurements *tpm_measure = (struct
							plat_tpm_measurements *)
							&bl_ctx->tpm_measurements;
	if (!tpm_measure) {
		printf("Failed to retrieve the boot measurements.\n");
		return -EPERM;
	}

	if (bl_ctx->tpm_hash_alg_type == OCS_HASH_SHA256) {
		debug("%s: SHA256 selected for measure boot hash alg\n", __func__);
		tpm_hash_alg = TPM2_ALG_SHA256;
	} else if (bl_ctx->tpm_hash_alg_type == OCS_HASH_SHA384) {
		debug("%s: SHA384 selected for measure boot hash alg\n", __func__);
		tpm_hash_alg = TPM2_ALG_SHA384;
	} else {
		printf("Invalid Hash algorithm \n");
		return -EINVAL;
	}

	rc = tpm_extend_pcr_and_log_event(dev, TPM_ROM_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->bl2_pcr0_value,
					  TPM_BL2_EVT_TYPE,
					  POST_CODE_STR_LEN,
					  EV_POSTCODE_INFO_POST_CODE);
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for bl2_pcr0_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_ROM_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->bl2_scp_pcr0_value,
					  TPM_BL2_EVT_TYPE,
					  POST_CODE_STR_LEN,
					  EV_POSTCODE_INFO_POST_CODE);
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for bl2_scp_pcr0_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_ROM_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->bl31_pcr0_value,
					  TPM_BL2_EVT_TYPE,
					  POST_CODE_STR_LEN,
					  EV_POSTCODE_INFO_POST_CODE);
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for bl31_pcr0_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_ROM_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->bl32_pcr0_value,
					  TPM_BL2_EVT_TYPE,
					  POST_CODE_STR_LEN,
					  EV_POSTCODE_INFO_POST_CODE);
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for bl32_pcr0_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_ROM_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->bl32ex1_pcr0_value,
					  TPM_BL2_EVT_TYPE,
					  POST_CODE_STR_LEN,
					  EV_POSTCODE_INFO_POST_CODE);
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for bl32ex1_pcr0_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_ROM_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->bl32ex2_pcr0_value,
					  TPM_BL2_EVT_TYPE,
					  POST_CODE_STR_LEN,
					  EV_POSTCODE_INFO_POST_CODE);
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for bl32ex2_pcr0_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_HPC_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->pdata_pcr1_value,
					  EV_TABLE_OF_DEVICES,
					  sizeof("THB_SOC_PDATA"),
					  "THB_SOC_PDATA");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for pdata_pcr1_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_HPC_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->tb_fw_cfg_pcr1_value,
					  EV_TABLE_OF_DEVICES,
					  sizeof("THB_SOC_TB_FW_CFG"),
					  "THB_SOC_TB_FW_CFG");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for tb_fw_cfg_pcr1_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_HPC_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->soc_fw_cfg_pcr1_value,
					  EV_TABLE_OF_DEVICES,
					  sizeof("THB_SOC_SOC_FW_CFG"),
					  "THB_SOC_SOC_FW_CFG");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for soc_fw_cfg_pcr1_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_HPC_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->tos_fw_cfg_pcr1_value,
					  EV_TABLE_OF_DEVICES,
					  sizeof("THB_SOC_TOS_FW_CFG"),
					  "THB_SOC_TOS_FW_CFG");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for tos_fw_cfg_pcr1_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_HPC_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->nt_fw_cfg_pcr1_value,
					  EV_TABLE_OF_DEVICES,
					  sizeof("THB_SOC_NT_FW_CFG"),
					  "THB_SOC_NT_FW_CFG");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for nt_fw_cfg_pcr1_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_HPC_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->boot_certs_pcr1_value,
					  EV_TABLE_OF_DEVICES,
					  sizeof("THB_SOC_BOOT_CERTS"),
					  "THB_SOC_BOOT_CERTS");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for boot_certs_pcr1_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_MBR_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->bl33_pcr4_value,
					  TPM_BL33_EVT_TYPE,
					  sizeof("THB_SOC_BL33"),
					  "THB_SOC_BL33");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for bl33_pcr4_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_SECURE_BOOT_POLICY_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->secure_boot_pcr7_value,
					  EV_ACTION,
					  sizeof("THB_SOC_SECURE_BOOT_BIT"),
					  "THB_SOC_SECURE_BOOT_BIT");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for secure_boot_pcr7_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_SECURE_BOOT_POLICY_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->alg_switch_pcr7_value,
					  EV_ACTION,
					  sizeof("THB_SOC_ALG_SWITCH_BIT"),
					  "THB_SOC_ALG_SWITCH_BIT");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for alg_switch_pcr7_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_SECURE_BOOT_POLICY_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->nvc_all_pcr7_value,
					  EV_ACTION,
					  sizeof("THB_SOC_NVC_ALL_BIT"),
					  "THB_SOC_NVC_ALL_BIT");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for nvc_all_pcr7_value failed ret %d\n", rc);
		return -EINVAL;
	}
	rc = tpm_extend_pcr_and_log_event(dev, TPM_SECURE_BOOT_POLICY_PCR_INDEX, tpm_hash_alg,
					  tpm_measure->trace_pcr7_value,
					  EV_ACTION,
					  sizeof("THB_SOC_TRACE_BIT"),
					  "THB_SOC_TRACE_BIT");
	if (rc) {
		printf("tpm_extend_pcr_and_log_event for trace_pcr7_value failed ret %d\n", rc);
		return -EINVAL;
	}

	return rc;
}

/**
 * get_bl1_ctx() - Get BL1 context.
 * @bl1_ctx: Where to store the retrieved context.
 *
 * Get the BL1 context (which includes various information, such as the boot
 * interface used by BL1) from the runtime monitor.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int get_bl1_ctx(platform_bl1_ctx_t *bl1_ctx)
{
	platform_bl1_ctx_t *shmem_bl1_ctx = NULL;
	int rc = 0;

	if (!bl1_ctx) {
		return -EINVAL;
	}

	shmem_bl1_ctx = get_secure_shmem_ptr(sizeof(*shmem_bl1_ctx));
	if (!shmem_bl1_ctx) {
		printf("No shared mem for communicating with secure world.\n");
		return -ENOMEM;
	}

	/* Set to zero */
	memset(shmem_bl1_ctx, 0x0, sizeof(*shmem_bl1_ctx));

	/* Request the information from the secure world */
	rc = sip_svc_get_bl1_ctx((uintptr_t)shmem_bl1_ctx,
				 sizeof(*shmem_bl1_ctx));
	if (rc) {
		printf("Failed to retrieve boot context.\n");
		return rc;
	}

	if (sizeof(*shmem_bl1_ctx) != shmem_bl1_ctx->ctx_size) {
		printf("Context size inconsistent.\n");
		printf("Definitions are out of sync.\n");
		return -EPROTO;
	}

	memcpy(bl1_ctx, shmem_bl1_ctx, sizeof(*bl1_ctx));

	return 0;
}

/*
 * Inform which mode is chosen, and set up the environment to trigger
 * the correct environment.
 */
static void set_boot_env_config(const char *boot_mode, const char *bootcmd,
				const char *bootargs)
{
	env_set("bootcmd", bootcmd);
	env_set("bootargs", bootargs);
}

/*
 * Configure the bootcmd for our board.
 */
static void setup_boot_mode(void)
{
	platform_bl1_ctx_t bl1_ctx;
	int rc;

	/* TODO: handle OS recovery mode once flow is defined. */
	rc = get_bl1_ctx(&bl1_ctx);
	/* TODO: U-boot is not able to get the context from Optee
	 * correctly. Porting to 64-bit is required.
	 */
	if (rc) {
		panic("Failed to retrieve bl1 ctx, Boot interface selection failed\n");
	} else  {
		boot_interface = bl1_ctx.boot_interface;
	}

	if (soc_rev == 0x00) {
		pr_info("THB(A0)");
	} else if (soc_rev == 0x01) {
		pr_info("THB(A1)");
	} else {
		//....
	}

	if (!boot_mode) {
		/* Open Boot*/
		env_set("verify", "0");
		pr_info("Open Boot\n");
	} else  {
		pr_info("Secure Boot\n");
		if (env_set_ulong("SECURE_SKU", 1) == 0) {
			pr_info("SECURE_SKU is set\n");
		}
	}

	switch (boot_interface) {
	case MA_BOOT_INTF_EMMC:
		pr_info("Boot Interface :eMMC\n");
		config_dtb_blob();
		set_boot_env_config("eMMC", THB_EMMC_BOOTCMD,
				    THB_EMMC_BOOTARGS);
		break;
	case MA_BOOT_INTF_PCIE:
		pr_info("Boot Interface :PCIe\n");
		config_dtb_blob();

		if (board_type_crb2 || board_type_hddl) { /*No eMMC for CRB2 / HDDL */
			set_boot_env_config("PCIe", THB_PCIE_BOOTCMD,
					    THB_PCIE_BOOTARGS);
		} else{
			/*we use eMMC only for Kernel Boot*/
			set_boot_env_config("eMMC", THB_EMMC_BOOTCMD,
					    THB_EMMC_BOOTARGS);
		}
		break;
	case MA_BOOT_INTF_SPI:
		break;
	}
}

static void ip_dis_clk(void)
{
	/* Disable PCIe RP, eMMC, ETH0 and ETH1 IP for EVT1, EVT2 and CRB2 boards */
	if ((board_id == BOARD_TYPE_HDDLF1) || (board_id == BOARD_TYPE_HDDLF2) || board_id == BOARD_TYPE_CRB2F1) {
		/* PCIe RP Reset */
		clrbits_32(PCIE_CPR_PCIE_RST_EN, BIT(4));
		/* PCIe Clock Gating */
		setbits_32(CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_45, BIT(6)|BIT(8)|BIT(10)|BIT(12)|BIT(14)|BIT(22)|BIT(24)|BIT(26)|BIT(28));
		clrbits_32(CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_45, BIT(7)|BIT(9)|BIT(11)|BIT(13)|BIT(15)|BIT(23)|BIT(25)|BIT(27)|BIT(29));
		setbits_32(CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_46, BIT(4)|BIT(8)|BIT(12));
		clrbits_32(CLKSS_PCIE_CLKSS_PCIE_APB_CTRL_46, BIT(5)|BIT(9)|BIT(13));

		/* ETH0 IP Reset */
		clrbits_32(PSS_CPR_ETHERNET_INST0_RST_EN, BIT(0));
		/* ETH0 Clock Gating */
		setbits_32(CLKSS_ETH_APB_CTRL_20, BIT(1));
		clrbits_32(CLKSS_ETH_APB_CTRL_20, BIT(0));
		setbits_32(CLKSS_ETH_APB_CTRL_21, BIT(1));
		clrbits_32(CLKSS_ETH_APB_CTRL_21, BIT(0));
		setbits_32(CLKSS_ETH_APB_CTRL_22, BIT(1));
                clrbits_32(CLKSS_ETH_APB_CTRL_22, BIT(0));
		setbits_32(CLKSS_ETH_APB_CTRL_23, BIT(1));
                clrbits_32(CLKSS_ETH_APB_CTRL_23, BIT(0));
		setbits_32(CLKSS_ETH_APB_CTRL_24, BIT(1));
                clrbits_32(CLKSS_ETH_APB_CTRL_24, BIT(0));

		/* ETH1 IP Reset */
		clrbits_32(PSS_CPR_ETHERNET_INST1_RST_EN, BIT(0));
		/* ETH1 Clock Gating */
		setbits_32(CLKSS_ETH_APB_CTRL_20, BIT(3));
		clrbits_32(CLKSS_ETH_APB_CTRL_20, BIT(2));
		setbits_32(CLKSS_ETH_APB_CTRL_21, BIT(3));
                clrbits_32(CLKSS_ETH_APB_CTRL_21, BIT(2));
		setbits_32(CLKSS_ETH_APB_CTRL_25, BIT(1));
		clrbits_32(CLKSS_ETH_APB_CTRL_25, BIT(0));
		setbits_32(CLKSS_ETH_APB_CTRL_22, BIT(3));
		clrbits_32(CLKSS_ETH_APB_CTRL_22, BIT(2));
		setbits_32(CLKSS_ETH_APB_CTRL_23, BIT(3));
		clrbits_32(CLKSS_ETH_APB_CTRL_23, BIT(2));

		setbits_32(PSS_CPR_MISC_CLK_REG_0, BIT(1));
		clrbits_32(PSS_CPR_MISC_CLK_REG_0, BIT(0));
		setbits_32(CLKSS_ETH_APB_CTRL_22, BIT(5));
                clrbits_32(CLKSS_ETH_APB_CTRL_22, BIT(4));

		/* eMMC IP Reset */
		clrbits_32(PSS_CPR_EMMC_RST_EN, BIT(0));
		/* eMMC Clock Gating */
		setbits_32(PSS_CPR_EMMC_CLK_GATING, BIT(1)|BIT(3)|BIT(5)|BIT(7));
		clrbits_32(PSS_CPR_EMMC_CLK_GATING, BIT(0)|BIT(2)|BIT(4)|BIT(6));
	}
}

int misc_init_r(void)
{
	setup_fdt();
	setup_boot_mode();
	ip_dis_clk();

	return 0;
}

int checkboard(void)
{
	puts("BOARD: Thunder Bay\n");

	return 0;
}

void reset_cpu(ulong addr)
{
	psci_system_reset();
}

int dram_init(void)
{
	gd->ram_size = get_effective_memsize();

	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = gd->ram_size;

	return 0;
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
#ifdef CONFIG_DEBUG_UART
	/* Enable debug UART */
	debug_uart_init();
#endif  /* CONFIG_DEBUG_UART */

	return 0;
}
#endif /* CONFIG_BOARD_EARLY_INIT_F */

static int thb_tpm_init(void)
{
	struct udevice *dev;
	int rc = 0;

	rc = get_tpm(&dev);
	if (rc) {
		return rc;
	}
	if (tpm_init(dev) || tpm2_startup(dev, TPM2_SU_CLEAR)) {
		printf("thb tpm_init failed\n");
		return -EINVAL;
	}

	if (tpm2_self_test(dev, TPMI_YES)) {
		printf("thb tpm self test failed\n");
		return -EINVAL;
	}

	return 0;
}

int board_init(void)
{
	return 0;
}

/*
 * This function is called during U-Boot initialization, after relocation (and
 * after board_init()).
 */
int board_early_init_r(void)
{
	return 0;
}

/**
 * board_is_secure() - Let us know if we are a secure SKU or not.
 *
 * Returns 1 for secure, otherwise 0
 */
static int board_is_secure(void)
{
	/* TODO: retrieve secure SKU. */
	return env_get_ulong("SECURE_SKU", 2, 0);
}

/**
 * apply_emmc_boot_partition_power_on_wp() - WP the boot partitions.
 *
 * Note that this function will hang the system if it fails to
 * write protect the boot partitions, because it is a requirement
 * of a secure system.
 *
 * Returns 0 for success, otherwise -ve error value
 */
static int apply_emmc_boot_partition_power_on_wp(void)
{
	struct mmc *mmc = find_mmc_device(0);

	pr_info("eMMC Boot Part WP\n");

	if (!mmc) {
		printf("Couldn't find eMMC device.\n");
		return -EIO;
	}
#ifdef THB_EMMC_DRIVER_UPGRADE
	/*
	 * Configuration is:
	 * - B_SEC_WP_SEL = 0
	 * - B_PWR_WP_DIS = 0
	 * - B_PERM_WP_DIS = 0
	 * - B_PERM_WP_SEC_SEL = 0
	 * - B_PERM_WP_EN = 0
	 * - B_PWR_WP_SEC_SEL = 0
	 * - B_PWR_WP_EN = 1
	 *
	 * i.e. Apply configuration to both boot partitions,
	 * configuration is: no permanent write protection, only
	 * power-on write protection.
	 */
	ret = mmc_set_boot_wp(mmc, 0, 0, 0, 0, 0, 0, 1);
	if (ret) {
		printf("Fatal error: failed to set BOOT_WP configuration.\n");
		return ret;
	}

	/* Verify above setup */
	ret = mmc_get_boot_wp_status(mmc, &boot_wp_status);
	if (ret) {
		printf("Fatal error: failed to get BOOT_WP_STATUS.\n");
		return ret;
	}

	if ((EXT_CSD_EXTRACT_B_AREA_1_WP(boot_wp_status) !=
	     EXT_CSD_BOOT_AREA_POWER_ON_PROTECTED) ||
	    (EXT_CSD_EXTRACT_B_AREA_2_WP(boot_wp_status) !=
	     EXT_CSD_BOOT_AREA_POWER_ON_PROTECTED)) {
		printf("Fatal: boot areas were not power-on protected.\n");
		return -EPERM;
	}
#endif

	return 0;
}

/*
 * As per Section 10 of TCG PC Client Platform Spec, update the auth
 * value for Platform hierarchy so that only firmware can use it.
 */
static int thb_tpm_change_platform_auth_hierarchy(void)
{
	char pwd_buffer[TPM2_DIGEST_LEN];
	u32 pwd_len = TPM2_DIGEST_LEN;
	u32 tpm_v2_handle = TPM2_RH_PLATFORM;
	int rc = 0, i = 0;
	struct udevice *dev;

	rc = get_tpm(&dev);
	if (rc) {
		return rc;
	}
	/*  random data is needed the password */
	srand(get_ticks() + rand());
	for (i = 0; i < TPM2_DIGEST_LEN; i++)
		pwd_buffer[i] = rand();

	rc = tpm2_change_auth(dev, tpm_v2_handle, (const char *)pwd_buffer,
			      pwd_len, NULL, 0);
	if (rc) {
		printf("%s: tpm2_change_auth failed ret %d\n", __func__, rc);
		return -EINVAL;
	}

	memset(pwd_buffer, 0x00, TPM2_DIGEST_LEN);

	return 0;
}

static int thb_tpm_close(void)
{
	int rc = 0;
	struct udevice *dev;

	rc = get_tpm(&dev);
	if (rc) {
		return rc;
	}

	if (tpm_deinit(dev)) {
		printf("deinitialize tpm failed\n");
		return -EINVAL;
	}

	return 0;
}

void board_preboot_os(void)
{
	ocs_hash_alg_t hash_alg;

	/* Update the PAD config for OS */
	pr_info("Applying pad configuration\n");
	pcl_pad_config(board_id, thb_full);

	if (board_is_secure()) {
		/*
		 * When the boot interface is eMMC, and we are a secure SKU,
		 * we must apply power-on write protection to the boot
		 * partitions.
		 */
		if (boot_interface == MA_BOOT_INTF_EMMC) {
			if (apply_emmc_boot_partition_power_on_wp()) {
				/*
				 * TODO: should centralise final state when some
				 * security issue has occurred, as opposed to
				 * other boot fails, etc.
				 */
				hang();
			}
		}

		/* Secure boot == yes, Measure boot == yes
		 * TPM init here
		 */
		if (thb_tpm_init()) {
			hang();
		}

		/* Get hash alg from bl ctx */
		if (measure_boot_bl2(&hash_alg)) {
			hang();
		}

		/* Use the same hash alg from kernel and FDT */
		if (measure_boot_kernel_fdt(hash_alg)) {
			hang();
		}

		/* Change TPM Platform Heirachy */
		if (thb_tpm_change_platform_auth_hierarchy()) {
			board_boot_fail(SECURITY_FAIL_TPM_CHANGE_PLAT_HIER);
		}

		if (thb_tpm_close()) {
			board_boot_fail(SECURITY_FAIL_TPM_DEINIT);
		}
#if defined(CONFIG_THUNDERBAY_MEM_PROTECT)
		if (BOARD_TYPE_HDDLF2 == board_id) {
			/* Below call will trigger SMC and call BL31 to setup
			 * Linux runtime firewall and lock the firewall setting.
			 */
			printf("%s: Firewall: Set Kernel Firewall\n", __func__);
			if (thb_imr_preboot_start()) {
				printf("%s: error: IMR setup failed\n", __func__);
				hang();
			}

			if (thb_imr_preboot_os()) {
				hang();
			}
		}
#endif  /* CONFIG_THUNDERBAY_MEM_PROTECT */
	}
}

void board_bootm_start(void)
{

}

phys_size_t get_effective_memsize(void)
{
	u8 ddr_mem[2] = { 0, 0 };
	u8 count = 0;
	int rc = 0;
	platform_bl_ctx_t plat_bl_ctx;

	/* Get BL context structure */
	rc = get_bl_ctx(&plat_bl_ctx);

	if (rc) {
		panic("Failed to retrieve bl ctx, slice and memory selection failed\n");
	}

	boot_mode = plat_bl_ctx.boot_mode; /* Update here to avoid calling bl_ctx multiple times*/
	board_id = plat_bl_ctx.board_id;
	soc_rev = plat_bl_ctx.soc_rev;
	measured_boot = plat_bl_ctx.measured_boot;
	ult_info = plat_bl_ctx.ult_info;
	memcpy(fdt_mrc_version, plat_bl_ctx.mrc_ver, MRC_VER_LEN);

	if (board_id == BOARD_TYPE_CRB2F1)  /* For Flashless Boot Configuration */
		board_type_crb2 = 1;

	if ((board_id == BOARD_TYPE_HDDLF1) || (board_id == BOARD_TYPE_HDDLF2))  /* For Flashless Boot Configuration */
		board_type_hddl = 1;

	/* Slice 0 Enable */
	if (plat_bl_ctx.slice_en[0]) {
		slice[0] = 1;
	}

	/* Slice 1 Enable */
	if (plat_bl_ctx.slice_en[1]) {
		slice[1] = 1;
	}

	/* Slice 2 Enable */
	if (plat_bl_ctx.slice_en[2]) {
		slice[2] = 1;
	}

	/* Slice 3 Enable */
	if (plat_bl_ctx.slice_en[3]) {
		slice[3] = 1;
	}

	/* If DDR CFG is 8GB */
	if (plat_bl_ctx.mem_id) {
		ddr_mem[1] = 1;
	} else {
		ddr_mem[0] = 1;/* If DDR CFG is 4GB */
	}
	thb_full =  (slice[0] & slice[1] & slice[2] & slice[3]);
	mem_id = plat_bl_ctx.mem_id;

	/* If configuration is Thunderbay Prime */
	if (!thb_full) {
		slice_mem_map[SLICE_0_2][SLICE_4GB] = slice[0] & slice[2] & ddr_mem[0];
		slice_mem_map[SLICE_0_3][SLICE_4GB] = slice[0] & slice[3] & ddr_mem[0];
		slice_mem_map[SLICE_1_2][SLICE_4GB] = slice[1] & slice[2] & ddr_mem[0];
		slice_mem_map[SLICE_1_3][SLICE_4GB] = slice[1] & slice[3] & ddr_mem[0];

		slice_mem_map[SLICE_0_2][SLICE_8GB] = slice[0] & slice[2] & ddr_mem[1];
		slice_mem_map[SLICE_0_3][SLICE_8GB] = slice[0] & slice[3] & ddr_mem[1];
		slice_mem_map[SLICE_1_2][SLICE_8GB] = slice[1] & slice[2] & ddr_mem[1];
		slice_mem_map[SLICE_1_3][SLICE_8GB] = slice[1] & slice[3] & ddr_mem[1];
	}

	/* If configuration is Thunderbay Full */
	if (thb_full) {
		slice_mem_map[SLICE_FULL][SLICE_4GB] = slice[0] & slice[1] & slice[2] & slice[3] & ddr_mem[0];
		slice_mem_map[SLICE_FULL][SLICE_8GB] = slice[0] & slice[1] & slice[2] & slice[3] & ddr_mem[1];
	}

	/* Total RAM Size */
	gd->ram_size = (plat_bl_ctx.dram_mem * SZ_1G) - SECURE_DDR_SIZE - SHARED_DDR_SIZE;

	total_mem = gd->ram_size;

	return gd->ram_size;
}

static const dtb_config_t dtb_configs_full[MEM_TYPE_MAX] = {
	{0, THB_FULL_4GB_DTB_CONF},
	{1, THB_FULL_8GB_DTB_CONF},
	{2, THB_FULL_8GB_DTB_CONF},
	{3, NULL},/* Not Handled Currently */
	{4, THB_FULL_8GB_4GB_DTB_CONF},
	{5, NULL},/* Not Handled Currently */
	{6, THB_FULL_8GB_4GB_DTB_CONF},
	{7, NULL},/* Not Handled Currently */
 };

static const dtb_config_t dtb_configs_prime[MEM_TYPE_MAX][PRIME_CONFIG] = {
	{{0, THB_PRIME_0_2_4GB_DTB_CONF}, {0, THB_PRIME_0_3_4GB_DTB_CONF}, {0, THB_PRIME_1_2_4GB_DTB_CONF}, {0, THB_PRIME_1_3_4GB_DTB_CONF} },
	{{1, THB_PRIME_0_2_8GB_DTB_CONF}, {1, THB_PRIME_0_3_8GB_DTB_CONF}, {1, THB_PRIME_1_2_8GB_DTB_CONF}, {1, THB_PRIME_1_3_8GB_DTB_CONF} },
	{{2, THB_PRIME_0_2_8GB_DTB_CONF}, {2, THB_PRIME_0_3_8GB_DTB_CONF}, {2, THB_PRIME_1_2_8GB_DTB_CONF}, {2, THB_PRIME_1_3_8GB_DTB_CONF} },
	{{3, NULL}, {3, NULL}, {3, NULL}, {3, NULL} }, /* Not Handled */
	{{4, THB_PRIME_0_2_8GB_4GB_DTB_CONF}, {4, THB_PRIME_0_3_8GB_4GB_DTB_CONF}, {4, THB_PRIME_1_2_8GB_4GB_DTB_CONF}, {4, THB_PRIME_1_3_8GB_4GB_DTB_CONF} },
	{{5, NULL}, {5, NULL}, {5, NULL}, {5, NULL} }, /* Not Handled */
	{{6, THB_PRIME_0_2_8GB_4GB_DTB_CONF}, {6, THB_PRIME_0_3_8GB_4GB_DTB_CONF}, {6, THB_PRIME_1_2_8GB_4GB_DTB_CONF}, {6, THB_PRIME_1_3_8GB_4GB_DTB_CONF} },
	{{7, NULL}, {7, NULL}, {7, NULL}, {7, NULL} }, /* Not Handled */
};

/* This functions configure the DTB blob name based on the THB Prime &
 * Memory density*/
void config_dtb_blob(void)
{
	env_set_ulong("board_id", board_id);

	if ((mem_id > 7) || (mem_id == 3) || (mem_id == 5) || (mem_id == 7))
		pr_info("%s- mem id Error\n", __func__);

	if (thb_full) {
		env_set("dtb_conf", dtb_configs_full[mem_id].dtb);
	} else {
		if (slice[0] & slice[2])	{
			env_set("dtb_conf", dtb_configs_prime[mem_id][PRIME_0_2_CFG].dtb);
			if (board_type_crb2 &&(mem_id == 0))
				env_set("dtb_conf", THB_PRIME_CRB2_0_2_4GB_DTB_CONF);
		} else if (slice[0] & slice[3]) {
			env_set("dtb_conf", dtb_configs_prime[mem_id][PRIME_0_3_CFG].dtb);
			if (board_type_crb2 &&(mem_id == 0))
				env_set("dtb_conf", THB_PRIME_CRB2_0_3_4GB_DTB_CONF);
		} else if (slice[1] & slice[2]) {
			env_set("dtb_conf", dtb_configs_prime[mem_id][PRIME_1_2_CFG].dtb);
			if (board_type_crb2 &&(mem_id == 0))
				env_set("dtb_conf", THB_PRIME_CRB2_1_2_4GB_DTB_CONF);
		} else if (slice[1] & slice[3]) {
			env_set("dtb_conf", dtb_configs_prime[mem_id][PRIME_1_3_CFG].dtb);
			if (board_type_crb2 &&(mem_id == 0))
				env_set("dtb_conf", THB_PRIME_CRB2_1_3_4GB_DTB_CONF);
		} else {
			pr_info("%s- Error\n", __func__);
			/* should not reach here*/
		}
	}

#if 0
	/* Following & get_effective_memsize() func will be cleaned up in separate Patch */
	if (slice_mem_map[SLICE_0_2][SLICE_4GB]) {
		if (board_type_crb2)
			env_set("dtb_conf", THB_PRIME_CRB2_0_2_4GB_DTB_CONF);
		else
			env_set("dtb_conf", THB_PRIME_0_2_4GB_DTB_CONF);
	} else if (slice_mem_map[SLICE_0_3][SLICE_4GB]) {
		if (board_type_crb2)
			env_set("dtb_conf", THB_PRIME_CRB2_0_3_4GB_DTB_CONF);
		else
			env_set("dtb_conf", THB_PRIME_0_3_4GB_DTB_CONF);
	} else if (slice_mem_map[SLICE_1_2][SLICE_4GB]) {
		if (board_type_crb2)
			env_set("dtb_conf", THB_PRIME_CRB2_1_2_4GB_DTB_CONF);
		else
			env_set("dtb_conf", THB_PRIME_1_2_4GB_DTB_CONF);
	} else if (slice_mem_map[SLICE_1_3][SLICE_4GB]) {
		if (board_type_crb2)
			env_set("dtb_conf", THB_PRIME_CRB2_1_3_4GB_DTB_CONF);
		else
			env_set("dtb_conf", THB_PRIME_1_3_4GB_DTB_CONF);
	} else if (slice_mem_map[SLICE_0_2][SLICE_8GB]) {
		env_set("dtb_conf", THB_PRIME_0_2_8GB_DTB_CONF);
	} else if (slice_mem_map[SLICE_0_3][SLICE_8GB]) {
		env_set("dtb_conf", THB_PRIME_0_3_8GB_DTB_CONF);
	} else if (slice_mem_map[SLICE_1_2][SLICE_8GB]) {
		env_set("dtb_conf", THB_PRIME_1_2_8GB_DTB_CONF);
	} else if (slice_mem_map[SLICE_1_3][SLICE_8GB]) {
		env_set("dtb_conf", THB_PRIME_1_3_8GB_DTB_CONF);
	}
	/* If configuration is Thunderbay Full */
	else if (slice_mem_map[SLICE_FULL][SLICE_4GB]) {
		env_set("dtb_conf", THB_FULL_4GB_DTB_CONF);
	} else if (slice_mem_map[SLICE_FULL][SLICE_8GB]) {
		env_set("dtb_conf", THB_FULL_8GB_DTB_CONF);
	} else   {
		pr_info("%s- Error\n", __func__);
		/* should not reach here*/
	}
#endif
	return;
}

int ddr_prof_setup_boot_args(void)
{
	if (ddr_prof_add_boot_args(thb_full, slice)) {
		pr_info("%s- Failed to add ddr profiling arguments\n", __func__);
		return 1;
	}

	return 0;
}

int tbh_os_svn_get(u32 *svn)
{
	u32 *boot_operation = NULL;
	u32 fuse_val = 0, bit_pos = 0, svn_idx = 0;
	u32 num_fuse_entries = 0;
	int i = 0;

	/* start and end index of os svn */
	if (SVN_START_ADDR >= MA_EFUSE_NUM_BITS / 64) {
		efuse_error_status(-EINVAL);
		return CMD_RET_FAILURE;
	}

	if (SVN_END_ADDR >= MA_EFUSE_NUM_BITS / 64) {
		efuse_error_status(-EINVAL);
		return CMD_RET_FAILURE;
	}

	if (SVN_END_ADDR < SVN_START_ADDR) {
		efuse_error_status(-EINVAL);
		return CMD_RET_FAILURE;
	}

	num_fuse_entries = (SVN_END_ADDR - SVN_START_ADDR) + 1;
	boot_operation = get_secure_shmem_ptr((sizeof(u32) * num_fuse_entries));
	if (!boot_operation) {
		log_err("Unable to retrieve shared memory.\n");
		return CMD_RET_FAILURE;
	}
	memset((void *)boot_operation, 0, (sizeof(u32) * num_fuse_entries));
	efuse_read_ranges(boot_operation, SVN_START_ADDR, SVN_END_ADDR);

	/* Find SVN value in fuse */
	for (i = 3; i >= 0; i--) {
		bit_pos = 0;
		fuse_val = boot_operation[i];
		if (fuse_val == 0) {
			svn_idx = svn_idx + 1;
			continue;
		}
		fuse_val = fuse_val/2;
		while (fuse_val != 0) {
			fuse_val = fuse_val/2;
			bit_pos++;
		}
		break;
	}

	if (svn_idx == 4)
		*svn = boot_operation[0] & BIT(0);
	else
		*svn = (((i * 32) + bit_pos) + 1);

	return 0;
}

int tbh_os_svn_set(u32 svn)
{
	u32 *fuse_mask, *boot_operation = NULL;
	u32 num_fuse_entries = 0, flags = 0;
	int i = 0;

	/* start and end index of os svn */
	if (SVN_START_ADDR >= MA_EFUSE_NUM_BITS / 64) {
		efuse_error_status(-EINVAL);
		return CMD_RET_FAILURE;
	}

	if (SVN_END_ADDR >= MA_EFUSE_NUM_BITS / 64) {
		efuse_error_status(-EINVAL);
		return CMD_RET_FAILURE;
	}

	if (SVN_END_ADDR < SVN_START_ADDR) {
		efuse_error_status(-EINVAL);
		return CMD_RET_FAILURE;
	}

	num_fuse_entries = (SVN_END_ADDR - SVN_START_ADDR) + 1;

	boot_operation = get_secure_shmem_ptr((sizeof(u32) * num_fuse_entries));

	if (!boot_operation) {
		log_err("Unable to retrieve shared memory.\n");
		return CMD_RET_FAILURE;
	}

	memset((void *)boot_operation, 0, (sizeof(u32) * num_fuse_entries));
	fuse_mask = boot_operation + (sizeof(u32) * num_fuse_entries);
	memset((void *)fuse_mask, 0, (sizeof(u32) * num_fuse_entries));

	if (svn != 0) {
		i = (svn - 1)/32;
		fuse_mask[i] = 1 << ((svn - 1)%32);
		efuse_write_ranges(boot_operation, fuse_mask, SVN_START_ADDR,
					SVN_END_ADDR, flags);
	}

	return 0;
}
