/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * sip_svc.h - Provides access to secure world services provided by ATF BL31.
 *
 * Copyright (c) 2018-2020 Intel Corporation
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

#ifndef _ARCH_THB_SIP_SVC_H_
#define _ARCH_THB_SIP_SVC_H_

#include <linux/arm-smccc.h>
#include <asm/arch/boot/ocs_hash.h>
#include <cpu_func.h>

#define THB_SIP_SVC_VPU_BOOT_FN_ID		(0x8200ff10)
#define THB_SIP_SVC_IMR_PCIE_SETUP		(0x8200ff12)
#define TMB_SIP_SVC_IMR_PCIE_CLEAR		(0x8200ff13)
#define THB_SIP_SVC_BOOT_IMR_KERNEL_RUNTIME	(0x8200ff14)
#define THB_SIP_SVC_BOOT_ECDSA_P384_VERIFY	(0x8200ff0B)
#define THB_SIP_SVC_BL1_CTX			(0x8200ff08)
#define THB_SIP_SVC_BL_CTX			(0x8200ff09)
#define THB_SIP_SVC_VPU_RESET_FN_ID		(0x8200ff15)
#define THB_SIP_SVC_VPU_STOP_FN_ID		(0x8200ff16)
#define THB_SIP_SVC_CLOCK_DEBUG_CONFIG		(0x8200ff17)
#define THB_SIP_SVC_BOOT_XMSS_VERIFY		(0x8200ff1d)
#define THB_SIP_SVC_BOOT_EFUSE_READ             (0x8200ff1f)
#define THB_SIP_SVC_BOOT_EFUSE_WRITE            (0x8200ff20)
#define THB_SIP_SVC_BOOT_EFUSE_PROVISION        (0x8200ff21)
#define THB_SIP_SVC_BOOT_EFUSE_READ_STATUS      (0x8200ff22)

#define EFUSE_PDATA_MAX_SIZE                    (0x400)

enum thb_imr {
	MA_IMR_0 = 0,
	MA_IMR_1,
	MA_IMR_2,
	MA_IMR_3,
	MA_IMR_4,
	MA_IMR_5,
	MA_IMR_6,
	MA_IMR_7,
	MA_IMR_8,
	MA_IMR_9,
	MA_IMR_10,
	MA_IMR_11,
	MA_IMR_12,
	MA_IMR_13,
	MA_IMR_14,
	MA_IMR_15,
	MA_IMR_MAX_NUM
};

/* Flag determines the behaviour during the programing of the fuses.
 * SOC_EFUSE_FLAG_READBACK - Will write back the value in the
 *                          buffer provided after fuses are programmed.
 * SOC_EFUSE_FLAG_NO_VERIFY - Will not perform verification after
 *                           programing the fuses, the default
 *                           behaviour is to verify.
 * SOC_EFUSE_FLAG_REPAIR     Will try to repair the fuse location in case of
 *                        programming failure,if repair bits available.
 * The flags can be OR'd together or used separately.
 */
enum soc_efuse_write_flags {
        SOC_EFUSE_NO_FLAG = 0,
        SOC_EFUSE_FLAG_READBACK,
        SOC_EFUSE_FLAG_NO_VERIFY,
        SOC_EFUSE_FLAG_REPAIR = 4
};

/* @sip_svc_imr_pcie_enable_firewall: Enable PCIE firewall registers
 * to allow access to DRAM region for PCIE boot.
*/

static inline int sip_svc_imr_pcie_enable_firewall()
{
        struct arm_smccc_res res = { 0 };

        arm_smccc_smc(THB_SIP_SVC_IMR_PCIE_SETUP, 0, 0, 0,
                      0, 0, 0, 0, &res);

        return res.a0;
}

/*
 * sip_svc_imr_pcie_disable_firewall() - Disable PCIE firewall register
 * to not allow access to DRAM region
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_imr_pcie_disable_firewall()
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(TMB_SIP_SVC_IMR_PCIE_CLEAR, 0, 0, 0, 0, 0, 0, 0,
		      &res);

	return res.a0;
}

/**
 * sip_svc_imr_protect_full_ddr() - Set up a special IMR protecting entire DDR.
 * Enable CPU master to access DRAM required for kernel booting.
 * Enable non-cpu master EMMC0 to access DRAM for kernel booting.
 * Enable PCIE to access DRAM region for Xlink to work.
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_imr_protect_full_ddr()
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(THB_SIP_SVC_BOOT_IMR_KERNEL_RUNTIME, 0, 0, 0, 0, 0, 0, 0,
		      &res);

	return res.a0;
}

/**
 * sip_svc_ecdsa_p384_verify() - Verify a digest against an ECDSA-384 signature.
 * @digest	  The digest to verify.
 * @digest_len	  The length of the dignest.
 * @public_key_qx The 'x' coordinate of the public key.
 * @public_key_qy The 'y' coordinate of the public key.
 * @signature_r   The 'r' coordinate of the signature.
 * @signature_y	  The 's' coordinate of the signature.
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_ecdsa_p384_verify(const uint8_t *digest,
					    size_t digest_len,
					    const uint8_t *public_key_qx,
					    const uint8_t *public_key_qy,
					    const uint8_t *signature_r,
					    const uint8_t *signature_s)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(THB_SIP_SVC_BOOT_ECDSA_P384_VERIFY,
		      (unsigned long)digest, (unsigned long)digest_len,
		      (unsigned long)public_key_qx,
		      (unsigned long)public_key_qy,
		      (unsigned long)signature_r, (unsigned long)signature_s,
		      0, &res);

	return res.a0;
}

/**
	 * sip_svc_xmss_verify() - Verify a digest against an XMSS signature.
	 * @dgst_ptr	The digest to verify.
	 * @dgst_len	The length of the digest.
	 * @sig_ptr	The signature.
	 * @sig_len	The length of the signature.
	 * @pubk_ptr	The XMSS public key.
	 * @pubk_len	The length of the public key.
	 *
	 * Return: 0 for success, anything else for failure.
	 */
	static inline int sip_svc_xmss_verify(const uint8_t *dgst_ptr, size_t dgst_len,
					      const uint8_t *sig_ptr, size_t sig_len,
					      const uint8_t *pubk_ptr, size_t pubk_len)
	{
		struct arm_smccc_res res = { 0 };

		flush_dcache_range((uintptr_t)dgst_ptr, (uintptr_t)dgst_ptr + dgst_len);
		flush_dcache_range((uintptr_t)sig_ptr, (uintptr_t)sig_ptr + sig_len);
		flush_dcache_range((uintptr_t)pubk_ptr, (uintptr_t)pubk_ptr + pubk_len);

		arm_smccc_smc(THB_SIP_SVC_BOOT_XMSS_VERIFY,
				(unsigned long)dgst_ptr,
				(unsigned long)dgst_len,
				(unsigned long)sig_ptr,
				(unsigned long)sig_len,
				(unsigned long)pubk_ptr,
				(unsigned long)pubk_len,
				0, &res);

		return res.a0;
	}

/*
 * sip_svc_get_bl1_ctx() - Retrieve BL1 context information
 * @buffer_paddr:	Physical address of buffer where we request the data
 *			be stored
 * @size:		Size of above buffer
 *
 * The BL1 context contains information about which peripheral was used
 * to boot and other context. The monitor will expect that the buffer
 * passed is located in the shared memory area.
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_get_bl1_ctx(uintptr_t buffer_paddr, uint64_t size)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(THB_SIP_SVC_BL1_CTX, (uint64_t)buffer_paddr, size, 0, 0,
		      0, 0, 0, &res);

	return res.a0;
}

/*
 * sip_svc_clock_debug_config() - Provide sip service for clock configuration
 * command
 * @clock_id:	clock_id
 * @en:		enable
 * @frequency:	frequency
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_clock_debug_config(u64 clock_id, u64 en,
					     u64 frequency)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(THB_SIP_SVC_CLOCK_DEBUG_CONFIG, clock_id, en, frequency,
		      0, 0, 0, 0, &res);

	return res.a0;
}

/*
 * sip_svc_get_bl_ctx() - Retrieve BL context information
 * @buffer_paddr:	Physical address of buffer where we request the data
 *			be stored
 * @size:		Size of above buffer
 *
 * The BL context contains information about measure boot info was used
 * to boot and other context. The monitor will expect that the buffer
 * passed is located in the shared memory area.
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_get_bl_ctx(uintptr_t buffer_paddr, uint64_t size)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(THB_SIP_SVC_BL_CTX, (uint64_t)buffer_paddr, size, 0, 0,
		      0, 0, 0, &res);

	return res.a0;
}


/*
 * sip_svc_efuse_read_ranges() - Read fuse values between indices given.
 * @boot_operation:             u32 pointer to buffer where the fuses values
 *                              would be read to.
 * @start_u32_idx:              starting index of the fuse to be read.
 * @end_u32_idx:                last index of fuse to be read.
 *
 * The boot_operation should be pre-allocated by user in the secure world
 * shared memory region.
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_efuse_read_ranges(const uintptr_t boot_operation,
                                            const uint32_t start_u32_idx,
                                            const uint32_t end_u32_idx)
{
        struct arm_smccc_res res = { 0 };

        flush_dcache_range((unsigned long)boot_operation,
                           (unsigned long)boot_operation +
                           (sizeof(u32) * ((end_u32_idx - start_u32_idx) + 1)));

        arm_smccc_smc(THB_SIP_SVC_BOOT_EFUSE_READ,
                      boot_operation,
                      start_u32_idx,
                      end_u32_idx,
                      0, 0, 0, 0, &res);

        return res.a0;
}



/*
 * sip_svc_efuse_write_ranges() - Write fuse values between indices given.
 * @boot_operation:             u32 pointer to buffer where the fuses values
 *                              would be read to.
 * @fuse_mask:                  u32 pointer to buffer containing the
 *                              fuse mask bits.
 * @start_u32_idx:              starting index of the fuse to be read.
 * @end_u32_idx:                last index of fuse to be read.
 * @flags:                      Flags for repair/verify behaviour.
 *
 * The boot_operation and fuse_mask should be pre-allocated by
 * user in the secure world
 * shared memory region.
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_efuse_write_ranges(const uintptr_t boot_operation,
                                             const uintptr_t fuse_mask,
                                             const uint32_t start_u32_idx,
                                             const uint32_t end_u32_idx,
                                             enum soc_efuse_write_flags flags)
{
        struct arm_smccc_res res = { 0 };

        flush_dcache_range((unsigned long)boot_operation,
                           (unsigned long)boot_operation +
                           (sizeof(u32) * ((end_u32_idx - start_u32_idx) + 1)));

        flush_dcache_range((unsigned long)fuse_mask,
                           (unsigned long)fuse_mask +
                           (sizeof(u32) * ((end_u32_idx - start_u32_idx) + 1)));

        arm_smccc_smc(THB_SIP_SVC_BOOT_EFUSE_WRITE,
                      boot_operation,
                      fuse_mask,
                      start_u32_idx,
                      end_u32_idx,
                      flags,
                      0, 0, &res);

        return res.a0;
}


/*
 * sip_svc_efuse_provision() - Provision fuses by reading the data BLOB.
 * @blob_address:               u32 pointer to fuse platform data.
 * @blob_size:                  Size of the blob.
 *
 * The blob_address should be pre-allocated by user in the secure world
 * shared memory region and should not exceed EFUSE_PDATA_MAX_SIZE.
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_efuse_provision(const uintptr_t blob_address,
                                          const size_t blob_size)
{
        struct arm_smccc_res res = { 0 };

        flush_dcache_range((unsigned long)blob_address,
                           (unsigned long)blob_address + EFUSE_PDATA_MAX_SIZE);

        arm_smccc_smc(THB_SIP_SVC_BOOT_EFUSE_PROVISION,
                      (const uintptr_t)blob_address,
                      (const size_t)blob_size, 0, 0, 0, 0, 0, &res);
        return res.a0;
}

/*
 * sip_svc_efuse_status() - Return back the efuse status.
 *
 * Return: Value in the efuse status register.
 */
static inline u32 sip_svc_efuse_status(void)
{
        struct arm_smccc_res res = { 0 };

        arm_smccc_smc(THB_SIP_SVC_BOOT_EFUSE_READ_STATUS,
                      0, 0, 0, 0, 0, 0, 0, &res);

        return (u32)res.a0;
}











#endif /* _ARCH_THB_SIP_SVC_H_ */
