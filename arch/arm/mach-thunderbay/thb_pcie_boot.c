// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (C) 2020 Intel Corporation.
 */
#include <common.h>
#include <command.h>
#include <spl.h>
#include <stdio.h>
#include <misc.h>
#include <memalign.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <asm/arch/thb_pcie_boot.h>
#include <asm/arch/thb_pcie.h>
#include <asm/arch/thb_shared_mem.h>
#include <part.h>
#include <blk.h>
#include <asm/io.h>
//#include drivers/misc/misc-uclass.c

#define PCIE_BOOT_OS_IMAGE (0)
#define PCIE_BOOT_FS_IMAGE (1)
#define PCIE_BOOT_HARDCODED_IMAGE_BASE_NUM (2)
#define PCIE_DOWNLOAD_BUFF_SIZE (0x100000 * 2)

#define PCIE_BOOT_BAR_MAP_SIZE (64 * 1024)

#define MF_DEST_MAX_SIZE (128)

/* Structure containing the context required to download an image. */
struct vpuuboot_image_ctx {
	u32 mf_ready;
	u32 mf_len;
	u8 reserved[8];
	u64 mf_start;
};

struct mmc_flash_ctx {
	u64 mf_offset;
	char mf_dest[MF_DEST_MAX_SIZE + 1];
};

static int pcie_command_done(struct udevice *devp)
{
	u32 mf_ready = VPUUBOOT_DOWNLOAD_DONE_VAL;
	int rc = 0;

	/* Indicate to the host the EP is ready for the next command. */
	rc = misc_write(devp, VPUUBOOT_MF_READY_OFFSET, &mf_ready,
			sizeof(mf_ready));
	if (rc) {
		pr_err("Failed to write MF_READY into communications registers.\n");
		return rc;
	}

	return rc;
}

static int pcie_download(struct udevice *devp, const u64 mf_start,
			 const u64 dest, const u32 mf_len, int terminate)
{
	int rc = 0;
	u32 mf_ready = 0;

	if (!mf_start) {
		log_err("MF_START retrieved from host as 0.\n");
		mf_ready = VPUUBOOT_INVALID_IMG_VAL;
		rc = misc_write(devp, VPUUBOOT_MF_READY_OFFSET, &mf_ready,
				sizeof(mf_ready));
		if (rc) {
			pr_err("Failed to write MF_READY error status into communications registers.\n");
			return rc;
		}
		return rc;
	}

	if (!mf_len) {
		log_err("Invalid MF_LEN retrieved from host.\n");
		mf_ready = VPUUBOOT_INVALID_IMG_VAL;
		rc = misc_write(devp, VPUUBOOT_MF_READY_OFFSET, &mf_ready,
				sizeof(mf_ready));
		if (rc) {
			pr_err("Failed to write MF_READY error status into communications registers.\n");
			return rc;
		}
		return rc;
	}

	if (!dest) {
		log_err("Invalid destination for transfer.\n");
		mf_ready = VPUUBOOT_INVALID_IMG_VAL;
		rc = misc_write(devp, VPUUBOOT_MF_READY_OFFSET, &mf_ready,
				sizeof(mf_ready));
		if (rc) {
			pr_err("Failed to write MF_READY error status into communications registers.\n");
			return rc;
		}
		return rc;
	}

	/* Indicate to the host the EP has received the start location and
	 * size successfully.
	 */
	mf_ready = VPUUBOOT_EP_START_VAL;
	rc = misc_write(devp, VPUUBOOT_MF_READY_OFFSET, &mf_ready,
			sizeof(mf_ready));
	if (rc) {
		pr_err("Failed to write MF_READY into communications registers.\n");
		return rc;
	}

	/* Perform the PCIe DMA. */
	rc = misc_call(devp, PCIE_BOOT_DMA_READ_ID, (void *)&mf_start,
		       sizeof(mf_start), (void *)dest, mf_len);
	if (rc) {
		log_err("DMA error. MF_START: 0x%llx MF_LEN 0x%x destination 0x%llx\n",
			mf_start, mf_len, dest);
		mf_ready = VPUUBOOT_ERR_VAL;
		rc = misc_write(devp, VPUUBOOT_MF_READY_OFFSET, &mf_ready,
				sizeof(mf_ready));
		if (rc) {
			pr_err("Failed to write MF_READY error status into communications registers.\n");
			return rc;
		}

		return rc;
	}

	if (terminate && rc == 0)
		rc = pcie_command_done(devp);

	return rc;
}

#ifdef CONFIG_CMD_THUNDERBAY_PCIE_RECOVERY_MMC
#ifndef CONFIG_PCIE_RECOVERY_MMC_DEV
#error "Require definition of CONFIG_PCIE_RECOVERY_MMC_DEV"
#endif /* CONFIG_PCIE_RECOVERY_MMC_DEV */
static int mmc_flash(struct udevice *devp,
		     struct vpuuboot_image_ctx *image_ctx, u64 mf_offset,
		     struct blk_desc *dev_desc, struct disk_partition *info)
{
	u32 pcie_offset = 0;
	void *download_buffer;
	unsigned long download_buffer_phys = 0;
	u32 download_length = 0;
	u32 blk_num = 0;
	u32 blk_diff = 0;
	u32 blk_written = 0;
	u64 length_written = 0;
	u64 mmc_offset = mf_offset / info->blksz;
	int terminate = 0;
	int rc = 0;
	int ret = CMD_RET_SUCCESS;

	download_buffer = malloc(PCIE_DOWNLOAD_BUFF_SIZE);

	if (!download_buffer) {
		log_err("No memory available for PCIe recovery buffer\n");
		return CMD_RET_FAILURE;
	}

	download_buffer_phys = virt_to_phys(download_buffer);

	while (image_ctx->mf_len > length_written) {
		if (image_ctx->mf_len > PCIE_DOWNLOAD_BUFF_SIZE) {
			download_length = PCIE_DOWNLOAD_BUFF_SIZE;
			terminate = 0;
		} else {
			download_length = image_ctx->mf_len;
			terminate = 1;
		}

		rc = pcie_download(devp, image_ctx->mf_start + pcie_offset,
				   (u64)download_buffer_phys, download_length,
				   terminate);
		if (rc) {
			log_err("Failed to download user defined image. Address: 0x%llx size: %x rc = %d\n",
				image_ctx->mf_start + pcie_offset,
				download_length, rc);
			ret = CMD_RET_FAILURE;
			break;
		}

		pcie_offset += download_length;
		/* Determine the number of blocks to write. */
		blk_num = BLOCK_CNT(download_length, dev_desc);
		if (download_length % info->blksz) {
			blk_diff = (blk_num * info->blksz) -
				   download_length;
			/* Clear the potentially extra data. */
			memset((void *)((u64)download_buffer +
			       (download_length % info->blksz)), 0, blk_diff);
		}

		if (info->size < (mmc_offset + blk_num)) {
			log_err("Image too large %llu for partition size %lu\n",
				mmc_offset + blk_num, info->size);
			ret = CMD_RET_FAILURE;
			break;
		}

		blk_written = blk_dwrite(dev_desc, info->start + mmc_offset,
					 blk_num, download_buffer);

		if (blk_written != blk_num) {
			log_err("Failure to write to mmc.\n");
			ret = CMD_RET_FAILURE;
			break;
		}

		mmc_offset += blk_written;
		length_written += blk_written * info->blksz;
	}

	free(download_buffer);

	return ret;
}

static int get_flash_ctx_from_host(struct udevice *devp,
				   struct blk_desc *dev_desc,
				   struct mmc_flash_ctx *flash_ctx,
				   struct disk_partition *info)
{
	int rc = 0;
	size_t len;

	/* Clear flash context before getting. */
	memset((void *)flash_ctx, 0, sizeof(*flash_ctx));
	rc = misc_read(devp, VPUUBOOT_MF_OFFSET_OFFSET,
		       flash_ctx, sizeof(*flash_ctx));
	if (rc) {
		pr_err("Failed to retrieve flashing context from communications registers. rc = %d\n",
		       rc);
		return CMD_RET_FAILURE;
	}

	len = strnlen(flash_ctx->mf_dest, sizeof(flash_ctx->mf_dest));
	if (len == sizeof(flash_ctx->mf_dest)) {
		pr_err("Received MF_DEST string is not NULL-terminated\n");
		return CMD_RET_FAILURE;
	}
	if (len >= MF_DEST_MAX_SIZE || len == 0) {
		pr_err("Invalid MF_DEST string length: %lu\n", len);
		return CMD_RET_FAILURE;
	}
	rc = part_get_info_by_name(dev_desc, flash_ctx->mf_dest,
				   info);
	if (rc < 0) {
		pr_err("Cannot get partition %s info.\n",
		       flash_ctx->mf_dest);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

/*
 * Perform PCIe recovery. Download an image from the host to the specified
 * partition given by MF_DEST and offset given by MF_OFFSET.
 */
static int do_pcie_recovery(struct cmd_tbl *cmdtp, int flag, int argc,
			    char * const argv[])
{
	/* Get boot context */
	platform_bl1_ctx_t bl1_ctx;
	struct vpuuboot_image_ctx image_ctx;
	struct blk_desc *dev_desc;
	struct udevice *devp;
	struct disk_partition info;
	struct mmc_flash_ctx flash_ctx;
	unsigned long erased = 0;
	char gpt_cmdbuf[32];
	struct pcie_iatu_setup_cfg iatu_cfg;
	char *magic = VPURECOV_MAGIC_STRING;
	u32 error_mf_ready;
	int rc = 0;
	int pr_count = 0;


	pr_info("%s-%d\n",__func__,CONFIG_PCIE_RECOVERY_MMC_DEV);

	/* Retrieve PCIe EP device. */
	rc = uclass_get_device_by_driver(UCLASS_MISC,
					 DM_DRIVER_GET(thb_pcie_ep), &devp);
	if (rc) {
		log_err("Error retrieving PCIe EP device. rc: %d\n", rc);
		return CMD_RET_FAILURE;
	}

	rc = get_bl1_ctx(&bl1_ctx);

	if (rc) {
		log_err("Error retrieving the BL CTX device. rc: %d\n", rc);
		return CMD_RET_FAILURE;
	}

	iatu_cfg.dev_id = bl1_ctx.efuse_device_id;
	iatu_cfg.comms_bar_size = PCIE_BOOT_BAR_MAP_SIZE;
	memcpy((void *)&iatu_cfg.magic, magic, sizeof(iatu_cfg.magic));
	rc = misc_ioctl(devp, PCIE_BOOT_IATU_SETUP, (void *)&iatu_cfg);
	if (rc < 0) {
		pr_err("Unable to setup iATU for PCIe driver. rc = %d\n", rc);
		return CMD_RET_FAILURE;
	}

	/* Enable the PCIe MISC driver. */
	rc = misc_set_enabled(devp, 1);
	if (rc < 0) {
		pr_err("Unable to enable PCIe driver. rc = %d\n", rc);
		return CMD_RET_FAILURE;
	}

	/* Get block desc from Fastboot flash mmc device
	 * which will always be present.
	 */
	dev_desc = blk_get_dev("mmc", CONFIG_PCIE_RECOVERY_MMC_DEV);
	if (!dev_desc) {
		pr_err("Block device not found.\n");
		goto exit;
	}

	/* Clear image context. */
	memset((void *)&image_ctx, 0, sizeof(image_ctx));

	/* While the host does not indicate boot. */
	do {
		rc = misc_read(devp, VPUUBOOT_MF_READY_OFFSET, &image_ctx,
			       sizeof(image_ctx));
		if (rc != CMD_RET_SUCCESS) {
			log_err("\nFailed to retrieve image context from communications registers.\n");

			break;
		}

		switch (image_ctx.mf_ready) {
		case VPUUBOOT_IMAGE_FLASH_GPT:
			rc = snprintf(gpt_cmdbuf, sizeof(gpt_cmdbuf),
				      "gpt write mmc %x $partitions",
				      CONFIG_PCIE_RECOVERY_MMC_DEV);
			if (rc < 0 || rc >= sizeof(gpt_cmdbuf)) {
				log_err("\nInternal error.\n");
				rc = CMD_RET_FAILURE;
				break;
			}
			rc = run_command(gpt_cmdbuf, 0);
			if (rc) {
				log_err("\nFailed to write partition table.\n");
				rc = CMD_RET_FAILURE;
				break;
			}
			if (pcie_command_done(devp)) {
				rc = CMD_RET_FAILURE;
				break;
			}
			break;
		case VPUUBOOT_IMAGE_PART_ERASE:
			rc = get_flash_ctx_from_host(devp, dev_desc,
						     &flash_ctx, &info);
			if (rc != CMD_RET_SUCCESS)
				break;
			/* Erase the partition. */
			erased = blk_derase(dev_desc, info.start, info.size);

			if (erased != info.size) {
				log_err("\nFailed to erase partition %s.\n",
					flash_ctx.mf_dest);
				rc = CMD_RET_FAILURE;
				break;
			}
			if (pcie_command_done(devp)) {
				rc = CMD_RET_FAILURE;
				break;
			}
			break;
		case VPUUBOOT_IMAGE_PART_FLASH:
			rc = get_flash_ctx_from_host(devp, dev_desc,
						     &flash_ctx, &info);
			if (rc != CMD_RET_SUCCESS)
				break;
			/* Download over PCIe and write to eMMC */
			rc = mmc_flash(devp, &image_ctx, flash_ctx.mf_offset,
				       dev_desc, &info);
			if (rc != CMD_RET_SUCCESS)
				break;

			/* Show a status update for large tx on screen. */
			printf(".");
			if (++pr_count % 79 == 0)
				printf("\n");
			break;
		default:
			break;
		}
	} while ((image_ctx.mf_ready != VPUUBOOT_EP_BOOT_VAL) &&
		 (rc == CMD_RET_SUCCESS));

	if (pr_count > 0)
		printf("\n");

	if (rc != CMD_RET_SUCCESS) {
		error_mf_ready = VPUUBOOT_INVALID_IMG_VAL;
		rc = misc_write(devp, VPUUBOOT_MF_READY_OFFSET, &error_mf_ready,
				sizeof(error_mf_ready));
		if (rc) {
			pr_err("Failed to write MF_READY error status into communications registers.\n");
			return rc;
		}
	}
exit:
	/* Disable the PCIe MISC driver, also disables iATU. */
	if (misc_set_enabled(devp, 0) < 0) {
		log_err("Unable to disable PCIe driver.\n");
		return CMD_RET_FAILURE;
	}

	return rc;
}

#endif /* CONFIG_CMD_THUNDERBAY_PCIE_RECOVERY_MMC */

/*
 * Perform PCIe boot. Download an image from the host to the specified
 * locations given by the arguments:
 * Argument 1 - Kernel image base address.
 * Argument 2 - RAM FS base address.
 */
static int do_pcie_boot(struct cmd_tbl *cmdtp, int flag, int argc,
			char * const argv[])
{
	/* Get boot context */
	platform_bl1_ctx_t bl1_ctx;
	u64 dest_address[PCIE_BOOT_HARDCODED_IMAGE_BASE_NUM] = {0};
	u64 offset[PCIE_BOOT_HARDCODED_IMAGE_BASE_NUM] = {0};
	int fs_retrieved = 0;
	struct udevice *devp;
	struct vpuuboot_image_ctx image_ctx;
	struct pcie_iatu_setup_cfg iatu_cfg;
	char *magic = VPUUBOOT_MAGIC_STRING;
	int rc;

	if (argc != 3) {
		pr_err("Unsupported number of arguments.\n");
		return CMD_RET_FAILURE;
	}

	pr_info("%s\n",__func__);

	/* Retrieve PCIe EP device. */
	rc = uclass_get_device_by_driver(UCLASS_MISC,
					 DM_DRIVER_GET(thb_pcie_ep), &devp);
	if (rc) {
		log_err("Error retrieving PCIe EP device. rc: %d\n", rc);
		return CMD_RET_FAILURE;
	}

	rc = get_bl1_ctx(&bl1_ctx);
	if (rc) {
		log_err("Error retrieving the BL CTX device. rc: %d\n", rc);
		return CMD_RET_FAILURE;
	}

	iatu_cfg.dev_id = bl1_ctx.efuse_device_id;
	iatu_cfg.comms_bar_size = PCIE_BOOT_BAR_MAP_SIZE;
	memcpy((void *)&iatu_cfg.magic, magic, sizeof(iatu_cfg.magic));

	/* Retrieve OS Image address. */
	dest_address[PCIE_BOOT_OS_IMAGE] = simple_strtoul(argv[1], NULL, 16);
	if (dest_address[PCIE_BOOT_OS_IMAGE] == 0) {
		pr_err("Invalid address for OS Image.\n");
		return CMD_RET_FAILURE;
	}

	/* Retrieve RAMFS address. */
	dest_address[PCIE_BOOT_FS_IMAGE] = simple_strtoul(argv[2], NULL, 16);
	if (dest_address[PCIE_BOOT_FS_IMAGE] == 0) {
		pr_err("FS base not provided.\n");
		return CMD_RET_FAILURE;
	}

	rc = misc_ioctl(devp, PCIE_BOOT_IATU_SETUP, (void *)&iatu_cfg);
	if (rc < 0) {
		pr_err("Unable to setup iATU for PCIe driver. rc = %d\n", rc);
		return CMD_RET_FAILURE;
	}
	/* Enable the PCIe MISC driver. */
	rc = misc_set_enabled(devp, 1);
	if (rc < 0) {
		pr_err("Unable to enable PCIe driver. rc = %d\n", rc);
		return CMD_RET_FAILURE;
	}

	/* Clear image context. */
	memset((void *)&image_ctx, 0, sizeof(image_ctx));

	/* While the host does not indicate boot. */
	do {
		rc = misc_read(devp, VPUUBOOT_MF_READY_OFFSET, &image_ctx,
			       sizeof(image_ctx));
		if (rc) {
			pr_err("Failed to retrieve image context from communications registers.\n");
			return CMD_RET_FAILURE;
		}

		switch (image_ctx.mf_ready) {
		case VPUUBOOT_IMAGE_OS:
			rc = pcie_download(devp, image_ctx.mf_start,
					   dest_address[PCIE_BOOT_OS_IMAGE] +
				offset[PCIE_BOOT_OS_IMAGE], image_ctx.mf_len,
				1);
			if (rc) {
				pr_err("Failed to download OS image.\n");
				return CMD_RET_FAILURE;
			}
			offset[PCIE_BOOT_OS_IMAGE] += image_ctx.mf_len;
			break;
		case VPUUBOOT_IMAGE_FS:
			rc = pcie_download(devp, image_ctx.mf_start,
					   dest_address[PCIE_BOOT_FS_IMAGE] +
				offset[PCIE_BOOT_FS_IMAGE], image_ctx.mf_len,
				1);
			if (rc) {
				pr_err("Failed to download FS image.\n");
				return CMD_RET_FAILURE;
			}
			fs_retrieved = 1;
			offset[PCIE_BOOT_FS_IMAGE] += image_ctx.mf_len;
			break;
		default:
			break;
		}
	} while (image_ctx.mf_ready != VPUUBOOT_EP_BOOT_VAL);

	if (!fs_retrieved) {
		/* No file system retrieved from the host, set to '-'. */
		env_set("initrd_addr", "-");
	}

	/* Disable the PCIe MISC driver, also disables iATU. */
	rc = misc_set_enabled(devp, 0);
	if (rc < 0) {
		pr_err("Unable to disable PCIe driver.\n");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(thb_pcie_boot, 3, 1, do_pcie_boot,
	   "Thunder Bay PCIe Boot sequence",
	   ""
);

#ifdef CONFIG_CMD_THUNDERBAY_PCIE_RECOVERY_MMC
U_BOOT_CMD(thb_pcie_emmc_recovery, 1, 1, do_pcie_recovery,
	   "Thunder Bay PCIe recovery of eMMC sequence",
	   ""
);
#endif /* CONFIG_CMD_THUNDERBAY_PCIE_RECOVERY_MMC */
