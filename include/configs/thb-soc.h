/*
 * (C) Copyright 2017 Linaro
 * Copyright (C) 2018-2020, Intel Corporation
 *
 * Jorge Ramirez-Ortiz <jorge.ramirez-ortiz@linaro.org>
 *
 * Configuration for Thunder Bay FPGA. Parts were derived from other ARM
 * configurations.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _THB_VP_H_
#define _THB_VP_H_

#include <linux/sizes.h>
#include <configs/thb-env.h>
#ifdef CONFIG_ENABLE_MENDER
#include <configs/thb-soc-mender.h>
#endif /* CONFIG_ENABLE_MENDER */

/* TBH Prime slice map and memory index */
#define SLICE_0_2       0
#define SLICE_0_3       1
#define SLICE_1_2       2
#define SLICE_1_3       3
#define SLICE_FULL      4

#define SLICE_4GB       0
#define SLICE_8GB       1

#define SLICE_INDEX	5
#define MEM_INDEX	2


/* DDR base address. */
#define DDR_BASE (0x1000000000)
/* Secure DDR usage. */
#define SECURE_DDR_SIZE (SZ_128M)
/* DDR shared between secure and non-secure world. */
#define SHARED_DDR_SIZE (SZ_32M)
#define SHARED_DDR_BASE (DDR_BASE + SECURE_DDR_SIZE)
/* SYS: hiding bottom of DDR from U-Boot */
#define CONFIG_SYS_SDRAM_BASE (DDR_BASE + SECURE_DDR_SIZE + SHARED_DDR_SIZE)
/* How much memory will U-Boot have available */
#define UBOOT_SDRAM_SIZE (0x800000000 - SECURE_DDR_SIZE - SHARED_DDR_SIZE)
/* Maximum uncompressed size of uImages (256MB). */
#define CONFIG_SYS_BOOTM_LEN (SZ_256M)
/* Size of early stack. */
#define INIT_STACK_SIZE (SZ_32K)
/*
 * Set initial stack pointer to be max. possible size of U-Boot + stack size
 * above the initial base address of U-Boot.
 */
#define CONFIG_SYS_INIT_SP_ADDR                                                \
	(CONFIG_SYS_SDRAM_BASE + SZ_4M + INIT_STACK_SIZE)
/* Default load address. */
#define CONFIG_SYS_LOAD_ADDR 0xB4C80000
/* Mallocable size. */
#define CONFIG_SYS_MALLOC_LEN SZ_4M
/* Thunder Bay fabric expects 32-bit accesses. */
#define CONFIG_SYS_NS16550_MEM32

/* SD/MMC */
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_SYS_MMC_MAX_BLK_COUNT 32767
#define CONFIG_SUPPORT_EMMC_BOOT

#define CONFIG_SYS_BOOT_RAMDISK_HIGH
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
#define CONFIG_SETUP_MEMORY_TAGS

/*****************************************************************************
 *  Boot configuration
 *****************************************************************************/

/*
 * eMMC configuration:
 *   kernel Image on partition 1
 *   initramfs.uboot on partition 2
 *   bootargs contains debug UART only
 *
 * TODO: remove initramfs part, remove initrd from booti
 * TODO: add root=/dev/mmcblk0pX parameter to bootargs
 */
#ifndef CONFIG_ENABLE_MENDER
#define THB_EMMC_BOOTCMD                                                       \
	"if test -e mmc 0:4 auto.scr; then echo found bootscript;load mmc 0:4 0x101C200000 auto.scr;source 0x101C200000; \
	 echo script loading failed: continuing...; fi;"\
        "mmc dev 0;echo loading Kernel...;echo load mmc 0:4 '$fit_addr' Image; load mmc 0:4 ${fit_addr} Image; \
		echo bootm '${fit_addr}'#${dtb_conf};bootm ${fit_addr}#${dtb_conf}\0"

/*
 * PCIe Configuration:
 *  Kernel image and ramfs will be loaded from host memory.
 *  bootargs contains debug UART only
 */
#define THB_PCIE_BOOTCMD                                                       \
        "thb_pcie_boot ${flashless_addr} ${ramfs_addr};"                   \
        "bootm ${flashless_addr}#${dtb_conf} ${ramfs_addr}\0"

#else /* CONFIG_ENABLE_MENDER */
#define THB_EMMC_BOOTCMD                      \
	"if test ${upgrade_available} = 1; "    \
        "then run mender_altbootcmd; "           \
	"else run mender_setup;"			\
        "fi;"					\
	"if test -e mmc 0:4 auto.scr; then echo found bootscript;load mmc 0:4 0x101C200000 auto.scr;source 0x101C200000; \
	 echo script loading failed: continuing...; fi;"\
        "mmc dev 0;echo loading Kernel...;echo load mmc 0:${mender_boot_part} '$fit_addr' Image; load mmc 0:${mender_boot_part} ${fit_addr} Image; \
		echo bootm '${fit_addr}'#${dtb_conf};bootm ${fit_addr}#${dtb_conf}\0" 	\
	"run mender_try_to_recover\0"

/*
 * PCIe Configuration:
 *  Kernel image and ramfs will be loaded from host memory.
 *  bootargs contains debug UART only
 */
#define THB_PCIE_BOOTCMD                                                       \
		"run mender_setup;" \
        "thb_pcie_boot ${flashless_addr} ${ramfs_addr};"                   \
        "bootm ${flashless_addr}#${dtb_conf} ${ramfs_addr}\0" 				\
		"run mender_try_to_recover\0"
#endif 	/* CONFIG_ENABLE_MENDER */

#define THB_EMMC_BOOTARGS "root=/dev/mmcblk0p${mender_rootfs_part} rootwait rw mender.data=PARTLABEL=data console=ttyS0,115200"
#define THB_PCIE_BOOTARGS "root=/dev/mem0 console=ttyS0,115200 rootwait rw rootfstype=ramfs"

/*
 * USB Configuration:
 * - Kernel image and ramfs will be loaded through fastboot (as a single FIT
 *   image)
 * - bootargs contains debug UART only
 */
#define THB_USB_BOOTCMD   \
        "fastboot 0; " \
        "bootm " __stringify(CONFIG_FASTBOOT_BUF_ADDR) " " \
                 __stringify(CONFIG_FASTBOOT_BUF_ADDR) " " \
                 "${fdt_addr}"
#define THB_USB_BOOTARGS "earlycon=uart8250,mmio32,0x80460000"
/*
 * Debug configuration:
 *   Assume image and ramdisk loaded to memory
 *   bootargs contains debug UART only
 */
#define THB_DEBUG_BOOTCMD "booti ${kernel_addr} ${initrd_addr} ${fdt_addr}"
#define THB_DEBUG_BOOTARGS "earlycon=uart8250,mmio32,0x80460000"
/*****************************************************************************
 *  Initial environment variables
 *****************************************************************************/

/* Env size is 32 kB -  env_mmc_nblks bytes */
//#define CONFIG_ENV_SIZE        THB_ENV_SIZE

#define BOOT_TARGET_DEVICES(func) func(MMC, mmc, 0)

/*****************************************************************************/
/*  eMMC partition configuration                                             */
/*****************************************************************************/

/* The following are standard GPT partition type GUIDs. */
#define GPT_TYPE_LINUX_ROOT_ARM64       "B921B045-1DF0-41C3-AF44-4C6F280D3FAE"
/* We use Linux FS for any partition that can be mounted, other than rootfs. */
#define GPT_TYPE_LINUX_FILESYSTEM       "0FC63DAF-8483-4772-8E79-3D69D8477DE4"
/* We use 'Linux Reserved' for any partition not supposed to be mounted. */
#define GPT_TYPE_LINUX_RESERVED         "8DA63339-0007-60C0-C436-083AC8230908"

/*
 * Macro to define a slot partition group.
 *
 * The 'slot' parameter should either be "a" or "b".
 *
 * Each group is composed of:
 * - a 'boot_<slot>' partition, containing the Linux boot partition for the
 *   slot.
 * - a 'system_<slot>' partition, containing the Linux root file-system for the
 *   slot.
 * - a 'syshash_<slot>' partition, containing the dm-verity root hash for the
 *   Linux root file-system partition.
 *
 * See 'doc/README.gpt' for the format used to describe the partitions.
 *
 * TODO: change the size of boot and system back to 512MB and 8GB respectively.
 */
#define THB_SLOT_PARTITION_GROUP(slot) \
                "name=boot_"    #slot ",size=256MB,"                          \
                                        "type=" GPT_TYPE_LINUX_FILESYSTEM ";" \
                "name=system_"  #slot ",size=5GB,"                            \
                                        "type=" GPT_TYPE_LINUX_ROOT_ARM64 ";" \
                "name=syshash_" #slot ",size=128MB,"                          \
                                        "type=" GPT_TYPE_LINUX_RESERVED ";"

/*
 * Macro defining the partitions used by Thunderbay Bay firmware:
 * - FIP Capsule Area (which starts at 1MB offset)
 * - The areas used to store U-boot environment (both main and backup); offset
 *   is explicit to prevent inconsistencies if THB_ENV_OFFSET is modified.
 */
#define THB_FW_PARTITIONS \
                "name=capsule,size=32MB,start=1MB,"                           \
                                        "type=" GPT_TYPE_LINUX_RESERVED ";"   \
                "name=env-main,size=" THB_ENV_SIZE_STR                    \
                                        ",start=" THB_ENV_OFFSET_STR      \
                                        ",type=" GPT_TYPE_LINUX_RESERVED ";"  \
                "name=env-redund,size=" THB_ENV_SIZE_STR                  \
                                        ",type=" GPT_TYPE_LINUX_RESERVED ";"

/*
 * Macro defining the minimal required eMMC partition table, composed of the FW
 * partitions and Slot A and B partition groups.
 *
 * The system will automatically enter OS recovery mode if this minimal set of
 * partitions is not present (or is different than expected).
 */
#define THB_PARTITION_TABLE_MINIMAL \
                THB_FW_PARTITIONS \
                THB_SLOT_PARTITION_GROUP(a) \
                THB_SLOT_PARTITION_GROUP(b)

/*
 * Macro defining the default eMMC partition table, composed of the required
 * partitions plus a final 'data' partition extending until the end of the
 * eMMC.
 *
 * This is the partition table flashed when the fastboot 'oem format' command
 * is received.
 */
#define THB_PARTITION_TABLE_DEFAULT \
                THB_PARTITION_TABLE_MINIMAL \
                "name=data,size=-,type=" GPT_TYPE_LINUX_FILESYSTEM


#ifndef CONFIG_SPL_BUILD
#include <config_distro_bootcmd.h>
#endif

#define THB_ENV_SETTINGS                                              \
	"loader_mmc_blknum=0x0\0"                                              \
	"loader_mmc_nblks=0x780\0"                                             \
	"env_mmc_blknum=0xf80\0"                                               \
	"env_mmc_blknum_redund=0xfc0\0"                                        \
	"env_mmc_nblks=0x40\0"                                                 \
	"bootargs=console=ttyS0,115200\0"    			                \
	"flashless_addr=0x1100000000\0"						\
	"ramfs_addr=0x101C200000\0"						\
	"fdt_high=0x101A1FFFFF\0"                                                \
	"initrd_addr=0x101A200000\0"                                             \
	"kernel_addr=0x100A000000\0"                                             \
        "kernel_addr_r=0x100A000000\0"                                           \
	"scriptaddr=0x100C000000\0"                                              \
	"fit_addr=0x105C200000\0"                                                \
	"dtb_addr=0x101A000000\0"						\
	"load_ramdisk=mmc read ${initrd_addr} 0x8000 0x1000\0"                 \
	"load_kernel=mmc read ${kernel_addr} 0x0 0x6000\0"                     \
	"boot_linux=booti ${kernel_addr} ${initrd_addr} ${fdt_addr}\0"         \
	"bootcmd=run load_ramdisk; run load_kernel; run boot_linux\0"          \
	"partitions=" THB_PARTITION_TABLE_DEFAULT "\0"                \
	"flash_part= gpt write mmc 0 ${partitions} \0"                \
	"initrd_high=0xffffffffffffffff\0" BOOTENV

#ifdef CONFIG_ENABLE_MENDER
#define CONFIG_EXTRA_ENV_SETTINGS  MENDER_ENV_SETTINGS THB_ENV_SETTINGS
#else
#define CONFIG_EXTRA_ENV_SETTINGS  THB_ENV_SETTINGS
#endif /*CONFIG_ENABLE_MENDER*/

//#define CONFIG_SYS_MMC_ENV_DEV THB_SYS_MMC_ENV_DEV
/* env_mmc_blknum bytes */
//#define CONFIG_ENV_OFFSET THB_ENV_OFFSET
/*
 * Redundant env is right after the main one: (0xfc0 * 512) --> 0x1f8000.
 *
 * Its location (in number of eMMC sectors) is stored in the
 * 'env_mmc_blknum_redund' env variable.
 */
//#define CONFIG_ENV_OFFSET_REDUND THB_ENV_OFFSET_REDUND
/* Use MMC partition zero to select whole user area of memory card. */
#define CONFIG_SYS_MMC_ENV_PART 0

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE 512
#define CONFIG_SYS_MAXARGS 64

/* Miscellaneous board init */
#define CONFIG_MISC_INIT_R 1

#endif /* _THB_VP_H_ */
