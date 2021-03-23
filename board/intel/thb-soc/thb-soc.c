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

#define GPIO_MICRON_FLASH_PULL_UP       20

#define THB_TPM_BL2_FROM_BL1_PCR_INDEX         0
#define THB_TPM_BL2_PCR_INDEX                   1
#define THB_TPM_FDT_PCR_INDEX                   2
#define THB_TPM_BL33_PCR_INDEX                  4
#define THB_TPM_KERNEL_PCR_INDEX                8

#define SZ_8G                           0x200000000

const char version_string[] = U_BOOT_VERSION_STRING CC_VERSION_STRING;

extern int get_tpm(struct udevice **devp);
static int get_bl_ctx(platform_bl_ctx_t *bl_ctx);

phys_size_t get_effective_memsize(void);

u8 board_type_crb2 __attribute__ ((section(".data")));
u8 board_type_hddl __attribute__ ((section(".data")));
u8 board_id __attribute__ ((section(".data")));

/* Mapping of TBH Prime Slices and Memory Cfg */
u8 slice_mem_map[SLICE_INDEX][MEM_INDEX] __attribute__ ((section(".data")));
u8 slice[4] __attribute__ ((section(".data")));
u8 thb_full __attribute__ ((section(".data")));
u8 mem_id __attribute__ ((section(".data")));

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
static uint8_t soc_rev __attribute__ ((section(".data")));

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
				ret = fdt_setprop_string(fdt, node, "status", "disabled");
			}
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
	ret = fdt_setprop_u32(fdt, hddl_dev_off, "board_type",
        	              plat_bl_ctx.board_id);
	if (ret) {
        	log_err("Failed to update board id in hddl_device node\n");
	        return ret;
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

	ret = fdt_thb_i2c_fixup(fdt);
	if (ret < 0) {
		log_err("Failed to update i2c-1 property\n");
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
	int rc;

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
		rc = tpm2_pcr_extend(dev, THB_TPM_KERNEL_PCR_INDEX, TPM2_ALG_SHA256, kernel_hash, TPM2_DIGEST_LEN);
	}
	if (hash_alg == OCS_HASH_SHA384) {
		printf("Measured boot SHA384: Writing to THB_TPM_KERNEL_PCR_INDEX ...\n");
		rc = tpm2_pcr_extend_sha384(dev, THB_TPM_KERNEL_PCR_INDEX, kernel_hash);
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
		rc = tpm2_pcr_extend(dev, THB_TPM_FDT_PCR_INDEX, TPM2_ALG_SHA256, fdt_hash, TPM2_DIGEST_LEN);
	}
	if (hash_alg == OCS_HASH_SHA384) {
		printf("Measured boot SHA384: Writing to THB_TPM_FDT_PCR_INDEX...\n");
		rc = tpm2_pcr_extend_sha384(dev, THB_TPM_FDT_PCR_INDEX, fdt_hash);
	}
	if (rc) {
		printf("%s: tpm2_pcr_extend failed ret %d\n", __func__, rc);
		return -EINVAL;
	}
	printf("Measured boot : Writing to THB_TPM_FDT_PCR_INDEX...SUCCESS\n");
	return 0;
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

	switch (bl_ctx->tpm_hash_alg_type) {
	case OCS_HASH_SHA256:
		debug("%s: hash alg %d tpm_secure_world_digest: %p\n",
		      __func__,
		      bl_ctx->tpm_hash_alg_type,
		      bl_ctx->tpm_secure_world_digest);

		printf("Measured boot SHA256: Writing to THB_TPM_BL2_FROM_BL1_PCR_INDEX...\n");
		rc = tpm2_pcr_extend(dev, THB_TPM_BL2_FROM_BL1_PCR_INDEX, TPM2_ALG_SHA256,
				     bl_ctx->tpm_secure_world_bl2_digest, TPM2_DIGEST_LEN);
		if (rc) {
			printf("%s: tpm2_pcr_extend failed ret %d\n",
			       __func__, rc);
			return -EINVAL;
		}

		printf("Measured boot : Writing to THB_TPM_BL2_FROM_BL1_PCR_INDEX...SUCCESS\n");
		printf("Measured boot SHA256: Writing to THB_TPM_BL2_PCR_INDEX...\n");

		rc = tpm2_pcr_extend(dev, THB_TPM_BL2_PCR_INDEX, TPM2_ALG_SHA256,
				     bl_ctx->tpm_secure_world_digest, TPM2_DIGEST_LEN);
		if (rc) {
			printf("%s: tpm2_pcr_extend failed ret %d\n",
			       __func__, rc);
			return -EINVAL;
		}
		printf("Measured boot : Writing to THB_TPM_BL2_PCR_INDEX... SUCCESS\n");
		debug("%s: tpm_normal_world_digest: %p\n", __func__,
		      bl_ctx->tpm_normal_world_digest);

		printf("Measured boot SHA256: Writing to THB_TPM_BL33_PCR_INDEX...\n");
		rc = tpm2_pcr_extend(dev, THB_TPM_BL33_PCR_INDEX, TPM2_ALG_SHA256,
				     bl_ctx->tpm_normal_world_digest, TPM2_DIGEST_LEN);
		if (rc) {
			printf("%s: tpm2_pcr_extend failed ret %d\n",
			       __func__, rc);
			return -EINVAL;
		}
		printf("Measured boot : Writing to THB_TPM_BL33_PCR_INDEX...SUCCESS\n");
		*hash_alg = bl_ctx->tpm_hash_alg_type;
		return 0;
	/* Only SHA256 is supported in tpm2_pcr_extend.
	 * SHA384 is not supported in dTPM Infineon chipset.
	 * HSD : https://hsdes.intel.com/appstore/article/#/1508191880
	 */
	case OCS_HASH_SHA384:
		/* platform_bl_ctx stores up hash size to SHA384_SIZE,
		 * SHA512 is not supported
		 */
		debug("%s: hash alg %d tpm_secure_world_digest: %p , tpm_secure_world_bl2_digest %p\n",
		      __func__,
		      bl_ctx->tpm_hash_alg_type,
		      bl_ctx->tpm_secure_world_digest,
		      bl_ctx->tpm_secure_world_bl2_digest);

		printf("Measured boot SHA384: Writing to THB_TPM_BL2_FROM_BL1_PCR_INDEX...\n");
		rc = tpm2_pcr_extend_sha384(dev, THB_TPM_BL2_FROM_BL1_PCR_INDEX,
					    bl_ctx->tpm_secure_world_bl2_digest);
		if (rc) {
			printf("%s: tpm2_pcr_extend failed ret %d\n",
			       __func__, rc);
			return -EINVAL;
		}

		printf("Measured boot : Writing to THB_TPM_BL2_FROM_BL1_PCR_INDEX...SUCCESS\n");
		printf("Measured boot SHA384: Writing to THB_TPM_BL2_PCR_INDEX...\n");
		rc = tpm2_pcr_extend_sha384(dev, THB_TPM_BL2_PCR_INDEX,
					    bl_ctx->tpm_secure_world_digest);
		if (rc) {
			printf("%s: tpm2_pcr_extend failed ret %d\n",
			       __func__, rc);
			return -EINVAL;
		}

		printf("Measured boot : Writing to THB_TPM_BL2_PCR_INDEX... SUCCESS\n");
		debug("%s: tpm_normal_world_digest: %p\n", __func__,
		      bl_ctx->tpm_normal_world_digest);

		printf("Measured boot SHA384: Writing to THB_TPM_BL33_PCR_INDEX...\n");
		rc = tpm2_pcr_extend_sha384(dev, THB_TPM_BL33_PCR_INDEX,
					    bl_ctx->tpm_normal_world_digest);
		if (rc) {
			printf("%s: tpm2_pcr_extend failed ret %d\n",
			       __func__, rc);
			return -EINVAL;
		}

		printf("Measured boot : Writing to THB_TPM_BL33_PCR_INDEX...SUCCESS\n");
		*hash_alg = bl_ctx->tpm_hash_alg_type;
		return 0;

	case OCS_HASH_SHA512:
	default:
		printf("%s: Invalid algorithm type\n", __func__);
		return -EINVAL;
	}

	return -EINVAL;
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

int misc_init_r(void)
{
	setup_fdt();
	setup_boot_mode();

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
#if defined(CONFIG_THUNDERBAY_MEM_PROTECT)
	if (thb_imr_post_u_boot_reloc()) {
		return -1;
	}
#endif  /* CONFIG_THUNDERBAY_MEM_PROTECT */

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
	if (board_id == BOARD_TYPE_HDDLF1) {
		pr_info("Applying EVT1 pad cfg\n");
		evt1_pad_config();
	}
	if (board_id == BOARD_TYPE_HDDLF2) {  /* For Flashless Boot Configuration */
		pr_info("Applying EVT2 pad cfg\n");
		evt2_pad_config();
	}

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
	}
	/* Below call will trigger SMC and call BL31 to setup Linux runtime firewall
         * and lock the firewall setting.
         */
	printf("Firewall: Set Kernel Firewall\n",__func__);
        if (thb_imr_preboot_start()) {
                printf("%s: error: IMR setup failed\n", __func__);
                hang();
        }

#if defined(CONFIG_THUNDERBAY_MEM_PROTECT)
	if (thb_imr_preboot_os()) {
		/* TODO: handle security issue properly. */
		hang();
	}
#endif  /* CONFIG_THUNDERBAY_MEM_PROTECT */
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
