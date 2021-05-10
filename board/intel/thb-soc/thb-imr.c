// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019-2020 Intel Corporation
 * Copyright (c) 1997-2005 Sean Eron Anderson
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

#include <common.h>
#include <image.h>
#include <asm/arch/access_control.h>
#include <asm/arch/sip_svc.h>

#include "thb-imr.h"

#define KERNEL_IMR	MA_IMR_2
#define FDT_IMR		MA_IMR_3
#define U_BOOT_IMR	MA_IMR_14
#define FULL_DDR_IMR	MA_IMR_15

/* IMR mask granting access to A53 only (in any mode). */
#define IMR_MASK_A53_ONLY (~(BIT(FIREWALL_A53_CPU_SECURE_INIT_SECURE_ID) | \
			   BIT(FIREWALL_A53_CPU_UNSECURE_INIT_SECURE_ID)))

/*
 * The maximum stack used by U-Boot. We should use a very conservative value
 * since we want to ensure that the IMR protecting U-Boot also protects the
 * stack.
 * Note: Measured stack size when booting signed kernel is 3680 bytes, so 2 MB
 * (more than 500 times greater than the measured usage) should be a reasonable
 * conservative value.
 */
#define THB_U_BOOT_MAX_STACK_SIZE	(2 * 1024 * 1024)

DECLARE_GLOBAL_DATA_PTR;

/*
 * Round up a number to a power of 2. Code taken from:
 * https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 */
static u64 roundup_pow2(u64 v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v++;

	return v;
}

/**
 * set_up_imr() - Set up an IMR protecting the specified memory range.
 * @imr:	The IMR to use.
 * @tgt_addr:	The base address of the target memory range to protect.
 * @tgt_size:	The size of the target memory range to protect.
 * @imr_base:	Where to store the computed IMR base address.
 * @imr_size:	Where to store the computed IMR size.
 *
 * Thunder Bay IMRs require the size of the IMR to be a power of 2 and the base
 * address to be aligned with the size.
 *
 * This helper function computes the base address and size of the smallest IMR
 * suitable for protecting the specified target memory range. In general, the
 * IMR will be larger than the target memory range (i.e., extra memory will be
 * protected). For this reason the base address and size of the IMR are
 * returned to the caller.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int set_up_imr(enum thb_imr imr, u64 tgt_addr, u64 tgt_size,
		      u64 *imr_base, u64 *imr_size)
{
	u64 base, size;

	debug("%s: target: base: %llx, size: %llx\n", __func__, tgt_addr,
	      tgt_size);

	if (tgt_size == 0) {
		debug("%s: invalid arguments\n", __func__);
		return -EINVAL;
	}
	base = tgt_addr;
	/* IMR size must be a power of 2; so round it up. */
	size = roundup_pow2(tgt_size);
	/* Check that IMR base is aligned to the IMR size. */
	while (!IS_ALIGNED(base, size)) {
		/* If not, align down IMR base address. */
		base = ALIGN_DOWN(base, size);
		/*
		 * Check that, with the new base address, we are still
		 * protecting the end of the target region.
		 */
		if ((tgt_addr + tgt_size) > (base + size)) {
			/*
			 * If not, increase the IMR size (i.e., double it, as
			 * it must remain a power of 2).
			 */
			size *= 2;
		}
	}
	debug("%s: IMR %d: base: %llx, size: %llx\n", __func__, imr, base,
	      size);
	*imr_base = base;
	*imr_size = size;

	return sip_svc_imr_setup(imr, base, size, IMR_MASK_A53_ONLY,
				 IMR_MASK_A53_ONLY);
}

/*
 * This function is called by board_early_init_r() (defined in thb-fpga.c),
 * which in turn is called during U-boot initialization, after relocation (and
 * after board_init()).
 *
 * We use it to:
 * - setup the IMR protecting the relocated U-Boot memory
 * - tear down the temporary IMR set up by BL2 to protect the entire DDR (thus
 *   making it accessible to boot media like eMMC, PCI, etc.).
 */
int thb_imr_post_u_boot_reloc(void)
{
	int rc = 0;

#if defined(CONFIG_THUNDERBAY_MEM_PROTECT_U_BOOT)
	u64 imr_base, imr_size, u_boot_base, u_boot_size;

	u_boot_base = gd->start_addr_sp - THB_U_BOOT_MAX_STACK_SIZE;
	u_boot_size = gd->ram_top - u_boot_base;

	/* TODO: IMR not setup for THB yet*/
#ifndef CONFIG_PLATFORM_THUNDERBAY
	rc = set_up_imr(U_BOOT_IMR, u_boot_base, u_boot_size, &imr_base,
			&imr_size);
	if (rc)
		return -1;
#endif
#endif /* defined(CONFIG_THUNDERBAY_MEM_PROTECT_U_BOOT) */

	/* Remove the global IMR set up by BL2. */
	/* TODO: For Thunderbay, IMR is disabled.*/
	return rc;
}

#if defined(CONFIG_THUNDERBAY_MEM_PROTECT)
/*
 * This function is called at the beginning of the bootm booting process (i.e.,
 * before the FIT is authenticated).
 *
 * We use it to set up the IMR protecting the entire DDR.
 */
int thb_imr_preboot_start(void)
{
	return sip_svc_imr_protect_full_ddr();
}

int thb_imr_pcie_enable_firewall()
{
	return sip_svc_imr_pcie_enable_firewall();
}

int thb_imr_pcie_disable_firewall()
{
	return sip_svc_imr_pcie_disable_firewall();
}
#endif /* CONFIG_THUNDERBAY_MEM_PROTECT */

/*
 * This function is called by board_preboot_os() (defined in thb_fpga.c),
 * which in turn is called right before the OS is booted.
 *
 * We perform the following actions:
 * - Setup the IMR protecting the kernel image.
 * - Setup the IMR protecting the FDT.
 * - Tear down the IMR protecting the entire U-Boot memory.
 * - Add U-Boot IMR number to the device tree (so that Linux can know which
 *   IMR must be cleared).
 */
int thb_imr_preboot_os(void)
{
        /*
         * Note: The 'images' variable is declared in image.h and defined in
         * bootm.c.
         */
        image_info_t *os = &images.os;
        u64 imr_base, imr_size;
        int rc;

        /* Protect the Kernel Image. */
        /* TODO: IMR not setup for THB yet*/
#ifndef CONFIG_PLATFORM_THUNDERBAY
        rc = set_up_imr(KERNEL_IMR, os->load, os->load_len, &imr_base,
                        &imr_size);
        if (rc) {
                puts("WARNING: failed to set up Kernel IMR.\n");
                return -1;
        }
#endif
        /*
         * Add the IMR to reserved memory table in the FDT.
         *
         * Note: FDT size has just been increased by CONFIG_SYS_FDT_PAD bytes
         * in boot_relocate_fdt(), so we are confident we have space for the
         * additional reserved memory entry.
         */

        rc = fdt_add_mem_rsv(images.ft_addr, imr_base, imr_size);
        if (rc) {
                puts("WARNING: failed to mark Kernel IMR as reserved memory\n");
                /* Not a critical error: continue. */
        }
#ifndef CONFIG_PLATFORM_THUNDERBAY
        /* Now protect Linux FDT. */
        rc = set_up_imr(FDT_IMR, (u64)images.ft_addr, images.ft_len, &imr_base,
                        &imr_size);
        if (rc) {
                puts("WARNING: failed to set up Device-Tree IMR.\n");
                return -1;
        }
        /* Add the IMR to reserved memory table in the FDT. */
        rc = fdt_add_mem_rsv(images.ft_addr, imr_base, imr_size);
        if (rc) {
                puts("WARNING: failed to mark FDT IMR as reserved memory.\n");
                /* Not a critical error: continue. */
        }
        /* TODO: For Thunderbay, IMR is disabled.*/
        /* Remove the global IMR we set at the start for bootm. */
        rc = sip_svc_imr_clear(FULL_DDR_IMR);
        if (rc) {
                /*
                 * Print a warning, but keep going as this is not a critical
                 * security issue.
                 */
                printf("WARNING: failed to remove global IMR: %d\n", rc);
        }

        /* Add U-Boot IMR property to 'chosen' node. */
        rc = fdt_add_imr_prop("u-boot-imr", U_BOOT_IMR);
        if (rc) {
                printf("WARNING: failed to add U-Boot IMR number to FDT.\n");
                /* Not a critical error: continue. */
        }

        /* FIXME: once IMR Linux Driver is ready, remove this. */
        /* TODO: For Thunderbay, IMR is disabled.*/
        sip_svc_imr_clear(U_BOOT_IMR);
#endif
        return 0;
}
