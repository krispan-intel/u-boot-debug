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
#define THB_SIP_SVC_IMR_SETUP			(0x8200ff12)
#define THB_SIP_SVC_IMR_CLEAR			(0x8200ff13)
#define THB_SIP_SVC_IMR_FULL_DDR_A53_ONLY	(0x8200ff14)
#define THB_SIP_SVC_BOOT_ECDSA_P384_VERIFY	(0x8200ff0B)
#define THB_SIP_SVC_BL1_CTX			(0x8200ff08)
#define THB_SIP_SVC_BL_CTX			(0x8200ff09)
#define THB_SIP_SVC_VPU_RESET_FN_ID		(0x8200ff15)
#define THB_SIP_SVC_VPU_STOP_FN_ID		(0x8200ff16)
#define THB_SIP_SVC_CLOCK_DEBUG_CONFIG		(0x8200ff17)
#define THB_SIP_SVC_BOOT_XMSS_VERIFY		(0x8200ff1d)

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

/**
 * sip_svc_imr_setup() - Set up an Isolated Memory Region (IMR)
 * @imr:		The IMR to be used.
 * @imr_base:		Base address of IMR (must be a multiple of imr_size).
 * @imr_size:		Size of IMR in bytes (must be a power of 2).
 * @init_rd_mask:	Initiator read mask (0 = access, 1 = no access).
 * @init_wr_mask:	Initiator write mask (0 = access, 1 = no access).
 *
 * The access policy for the provided IMR will be configured for all initiators
 * in the system by means of the provided read/write bit masks.
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_imr_setup(enum thb_imr imr, uint64_t imr_base,
				    uint64_t imr_size, uint64_t init_rd_mask,
				    uint64_t init_wr_mask)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(THB_SIP_SVC_IMR_SETUP, imr, imr_base, imr_size,
		      init_rd_mask, init_wr_mask, 0, 0, &res);

	return res.a0;
}

/*
 * sip_svc_imr_clear() - Clear an Isolated Memory Region (IMR).
 * @imr: The IMR to be cleared.
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_imr_clear(enum thb_imr imr)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(THB_SIP_SVC_IMR_CLEAR, imr, 0, 0, 0, 0, 0, 0,
		      &res);

	return res.a0;
}

/**
 * sip_svc_imr_protect_full_ddr() - Set up a special IMR protecting entire DDR.
 * @imr: The IMR to be used.
 *
 * The specified IMR is configured to cover the entire DDR and allow access to
 * A53 only (in any mode).
 *
 * Return: 0 for success, anything else for failure.
 */
static inline int sip_svc_imr_protect_full_ddr(enum thb_imr imr)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(THB_SIP_SVC_IMR_FULL_DDR_A53_ONLY, imr, 0, 0, 0, 0, 0, 0,
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

#endif /* _ARCH_THB_SIP_SVC_H_ */
