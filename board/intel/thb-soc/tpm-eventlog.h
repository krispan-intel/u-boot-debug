/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019, Intel Corporation.
 */

#ifndef __TPM_EVENTLOG_H
#define __TPM_EVENTLOG_H

#include <hash.h>
#include <tpm-v2.h>

//#define EV_NO_ACTION			0x03
//#define EV_POST_CODE			0x1
#define EV_PLATFORM_CONFIG_FLAGS	((u32)0x0000000A)
//#define EV_TABLE_OF_DEVICES		0xB
#define EV_COMPACT_HASH			((u32)0x0000000C)
//#define EV_SEPARATOR			0x4
#define EV_ACTION			((u32)0x00000005)

#define EV_EFI_VARIABLE_DRIVER_CONFIG	0x80000001
#define EV_EFI_GPT_EVENT		0x80000006

/* update depending on how many hash alg supported in future */
#define HASH_ALG_COUNT			1

#define TCG_EVENT_NAME_LEN_MAX		255

#define TCG_EFI_SPEC_ID_SIGNATURE_03	"Spec ID Event03"
#define TCG_EFI_STARTUP_LOCALITY_EVENT_SIGNATURE	"StartupLocality"

#define TCG_EFI_SPEC_VERSION_MAJOR_TPM2   2
#define TCG_EFI_SPEC_VERSION_MINOR_TPM2   0
#define TCG_EFI_SPEC_ERRATA_TPM2          0

#define EV_POSTCODE_INFO_POST_CODE    "POST CODE"
#define POST_CODE_STR_LEN             (sizeof(EV_POSTCODE_INFO_POST_CODE) - 1)

enum locality_event_index {
	TCG_EFI_STARTUP_LOCALITY_EVENT = 0,
	ATF_EVENT,
	OPTEE_EVENT,
	UBOOT_EVENT
};

/**
 * tpm_2_0_log_init()
 *
 * Perform tpm 2.0 event log initiailazation
 *
 * @lasa_p: pointer to lasa address
 * @laml: the value of laml
 *
 * Return: 0 on success, negative error code otherwise.
 */
int tpm_2_0_log_init(u32 *lasa_p, u32 laml);

/**
 * tpm_extend_pcr_and_log_event()
 *
 *   Extend a PCR and add a new entry into tpm 2.0 event log.
 *
 * @dev: tpm device
 * @pcr_index: pcr index id
 * @hash_alg: hash algorithm: currently only support SHA256
 * @hash: pointer to the input hash value
 * @event_type: EFI event type
 * @event_size: event info size in bytes
 * @event: pointer to the event info.
 *
 * Return: 0 on success, negative error code otherwise.
 */

int tpm_extend_pcr_and_log_event(struct udevice *dev, int pcr_index,
				 enum tpm2_algorithms hash_alg, const u8 *hash,
				 u32 event_type, size_t event_size,
				 const char *event);

/**
 * tpm_log_locality_event()
 *
 * Add locality event into tpm 2.0 event log
 *
 * @startup_locality: TPM startup locality
 * @signature: TPM Signature string
 *
 * Return: 0 on success, negative error code otherwise.
 */
int tpm_log_locality_event(u8 startup_locality, const char *signature);

#endif /* __TPM_EVENTLOG_H */
