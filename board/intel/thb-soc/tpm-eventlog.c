// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021, Intel Corporation.
 */

#include <fdtdec.h>
#include <fdt_support.h>
#include <log.h>
#include <tpm-v2.h>
#include <version.h>

#include <linux/errno.h>

#include <u-boot/sha1.h>
#include <u-boot/sha256.h>

#include "tpm-eventlog.h"

/* Disable data struct padding */
#pragma pack(push, 1)
struct tcg_efi_startup_locality_event {
	u8 signature[16];
	u8 startup_locality;
};

struct tcg_efi_spec_id_event_algorithm_size {
	u16 algorithm_id;
	u16 digest_size;
};

struct tcg_efi_spec_id_event_struct {
	u8 signature[16];
	u32 platform_class;
	u8 spec_version_minor;
	u8 spec_version_major;
	u8 spec_errata;
	u8 uintn_size;
	u32 number_of_algorithms;
	struct tcg_efi_spec_id_event_algorithm_size digest_size[HASH_ALG_COUNT];
	u8 vendor_info_size;
	u8 vendor_info[];
};

struct digest_val_1_2 {
	u8 digest[SHA1_SUM_LEN];
};

#if 0
struct tcg_pcr_event {
	u32	pcr_index;
	u32	event_type;
	struct	digest_val_1_2 digests;
	u32	event_size;
	u8	event[];	/* event data */
};
#endif

/* tpm 2.0 support more hash algo, but we only support sha256 */
struct tpm_hash_2_0 {
	u8 sha256[SHA256_SUM_LEN];
};

struct tpm_hash_2_0_st {
	u16 hash_alg;
	struct tpm_hash_2_0 digest;
};

struct digest_val_2_0 {
	u32  count;
	struct tpm_hash_2_0_st digests[HASH_ALG_COUNT];
};

struct tcg_pcr_event2_hdr {
	u32	pcr_index;
	u32	event_type;
	struct	digest_val_2_0 digests;
	u32	event_size;
};

#pragma pack(pop)

static u32 *tpm_lasa_p;
static u32 tpm_laml;

/*
 * Allocate and initialize TCG Event Log.
 *
 * See TCG PC client Platform Firmware Profile Specification, section 9.0 event
 * logging for design spec:
 *
 * |---------------------------|
 * |---------PCR Index---------|
 * |--------Event Type---------|
 * |--------SHA1 Digest--------|
 * |-------Event Data Size-----|
 * ||-------Event Data--------||
 * ||------spec_id_event------||
 * ||-------digest_size-------||
 * ||-----vendor_info_size----||
 * |---------------------------|
 *
 * |---------------------------|
 * |---------PCR Index---------|
 * |--------Event Type---------|
 * |-------Digest.count--------|
 * |-Digests.digests[0].alg_id-|
 * |-Digests.digests[0].digest-|
 * |-------Event Data Size-----|
 * |--------Event Data---------|
 * |---------------------------|
 *
 */
int tpm_2_0_log_init(u32 *lasa_p, u32 laml)
{
	int rc = 0;
	struct tcg_pcr_event *first_pcr_event;
	struct tcg_efi_spec_id_event_struct *spec_id_event;
	struct tcg_efi_spec_id_event_algorithm_size *digest_size;

	tpm_lasa_p = lasa_p;
	tpm_laml = laml;

	printf("INFO: TPM 2.0 event log init: lasa %p laml %d\n",
	       tpm_lasa_p, tpm_laml);
	/* initialise the log space */
	memset((void *)tpm_lasa_p, 0, tpm_laml);
	first_pcr_event = (struct tcg_pcr_event *)tpm_lasa_p;
	first_pcr_event->pcr_index = 0;
	first_pcr_event->event_type = EV_NO_ACTION;

	spec_id_event = (struct tcg_efi_spec_id_event_struct *)
			first_pcr_event->event;

	debug("%s sizeof first_pcr_event %ld\n",
	      __func__, sizeof(*first_pcr_event));

	if (strlcpy((char *)spec_id_event->signature,
		    (const char *)&TCG_EFI_SPEC_ID_SIGNATURE_03,
		    sizeof(spec_id_event->signature)) >
	    sizeof(spec_id_event->signature)) {
		pr_err("TPM Event Signature buffer overflow.\n");
		return -ENOMEM;
	}

	spec_id_event->platform_class = 0;
	spec_id_event->spec_version_minor =
		TCG_EFI_SPEC_VERSION_MINOR_TPM2;
	spec_id_event->spec_version_major = TCG_EFI_SPEC_VERSION_MAJOR_TPM2;
	spec_id_event->spec_errata = TCG_EFI_SPEC_ERRATA_TPM2;
	spec_id_event->uintn_size = 2;
	spec_id_event->number_of_algorithms = HASH_ALG_COUNT;

	digest_size = spec_id_event->digest_size;
	digest_size->algorithm_id = TPM2_ALG_SHA256;
	digest_size->digest_size = SHA256_SUM_LEN;

	spec_id_event->vendor_info_size = 0;
	first_pcr_event->event_size = sizeof(*spec_id_event);

	debug("%s done: first_pcr_event->event_size %d\n", __func__,
	      first_pcr_event->event_size);

	return rc;
}

static void add_event_tcg_log(u8 *loc, struct tcg_pcr_event2_hdr *event_hdr,
			      const char *event_data)
{
	debug("adding event in tcg event log at : 0x%p, size: %ld\n",
	      loc, sizeof(struct tcg_pcr_event2_hdr));
	/* copy tcg_pcr_hdr first */
	/* in the tcg_pcr_event2_hdr is a fix size as only sha256 is
	 * supported in uboot.
	 */
	memcpy(loc, event_hdr, sizeof(struct tcg_pcr_event2_hdr));
	loc += sizeof(struct tcg_pcr_event2_hdr);

	/* copy event data */
	memcpy(loc, event_data, event_hdr->event_size);
	printf("INFO: TPM 2.0 event log add entry: %s\n", event_data);
}

static u32 get_compressed_tcg_event_size(struct tcg_pcr_event2_hdr *event_hdr)
{
	u32 total_size = 0;

	/* check if the event hdr is empty */
	if (event_hdr->event_type == 0 && event_hdr->event_size == 0 &&
	    event_hdr->digests.count == 0)
		return 0;

	total_size = sizeof(struct tcg_pcr_event2_hdr) + event_hdr->event_size;

	debug("%s: total_size 0x%8x\n", __func__, total_size);
	return total_size;
}

static int tpm_log_event(struct tcg_pcr_event2_hdr *event_hdr,
			 const char *event_data)
{
	u32 *event_size;
	u32 log_size;
	u32 *max_size;

	struct tcg_pcr_event2_hdr *empty_slot;
	struct tcg_pcr_event *first_slot;

	if (!tpm_lasa_p || tpm_laml == 0) {
		pr_err("unable to get log area for tcg 2.0 format events!\n");
		return -EINVAL;
	}

	max_size = (u32 *)((u8 *)tpm_lasa_p + tpm_laml - 1);

	first_slot = (struct tcg_pcr_event *)(tpm_lasa_p);
	empty_slot = (struct tcg_pcr_event2_hdr *)
		((u8 *)first_slot->event + first_slot->event_size);

	debug("%s: sizeof tcg_pcr_event %ld, first_slot->event_size %d\n",
	      __func__, sizeof(struct tcg_pcr_event),
	      first_slot->event_size);

	debug("%s: first empty slot at %p\n", __func__, empty_slot);
	while (empty_slot < (struct tcg_pcr_event2_hdr *)max_size) {
		log_size = get_compressed_tcg_event_size(empty_slot);
		if (log_size > 0) {
			empty_slot = (struct tcg_pcr_event2_hdr *)
				((u8 *)empty_slot + log_size);
			debug("%s: update empty slot to %p\n", __func__,
			      empty_slot);
		} else {
			break;
		}
		/* Don't care if go beyond max_size */
	}

	/* before adding new event, check if there is enough space available. */
	event_size = (u32 *)((u8 *)empty_slot +
			     (sizeof(struct tcg_pcr_event2_hdr) +
			      event_hdr->event_size));
	if (event_size <= max_size) {
		add_event_tcg_log((u8 *)empty_slot, event_hdr, event_data);
	} else {
		pr_err("insufficient space: event not logged!\n");
		return -ENOMEM;
	}
	return 0;
}

int tpm_extend_pcr_and_log_event(struct udevice *dev, int pcr_index,
				 enum tpm2_algorithms hash_alg,
				 const u8 *hash, u32 event_type,
				 size_t event_size, const char *event)
{
	int rc = 0;
	struct tcg_pcr_event2_hdr pcr_event_hdr;
	struct digest_val_2_0	*digests;

	u8 zero[SHA256_SUM_LEN] = {0};

	/* we only support sha256 in u-boot*/
	if (hash_alg != TPM2_ALG_SHA256)
		return -EINVAL;

	if (event_size > TCG_EVENT_NAME_LEN_MAX)
		return -EINVAL;

	if (!hash)
		return -EINVAL;

	if (memcmp(hash, zero, SHA256_SUM_LEN) == 0) {
		log_warning("Hash is zero, wouldn't extend to PCR\n");
		return 0;
	}

	rc = tpm2_pcr_extend(dev, pcr_index, hash_alg, hash, SHA256_SUM_LEN);
	if (rc) {
		log_err("tpm2_pcr_extend failed ret %d\n", rc);
		return -EINVAL;
	}

	digests = &pcr_event_hdr.digests;
	digests->count = 1;
	digests->digests[0].hash_alg = hash_alg;

	memcpy(&digests->digests[0].digest, hash, SHA256_SUM_LEN);

	pcr_event_hdr.pcr_index = pcr_index;
	pcr_event_hdr.event_type = event_type;
	pcr_event_hdr.event_size = event_size;

/*
	rc = tpm_log_event(&pcr_event_hdr, event);
	if (rc) {
		pr_err("PCR (%u) log failed with error %d\n",
		       pcr_index, rc);
	}
*/
	return rc;
}

int tpm_log_locality_event(u8 startup_locality, const char *signature)
{
	int rc = 0;
	struct tcg_efi_startup_locality_event startup_locality_event;
	struct tcg_pcr_event2_hdr pcr_event_hdr;
	struct digest_val_2_0   *digests;

	debug("%s %d:%s\n", __func__, startup_locality, signature);
	if (strlcpy((char *)&startup_locality_event.signature, signature,
		    sizeof(startup_locality_event.signature)) >
	    sizeof(startup_locality_event.signature)) {
		pr_err("TPM Event Signature buffer overflow.\n");
		return -ENOMEM;
	}

	startup_locality_event.startup_locality = startup_locality;

	pcr_event_hdr.pcr_index = 0;
	pcr_event_hdr.event_type = EV_NO_ACTION;
	pcr_event_hdr.event_size = sizeof(startup_locality_event);

	digests = &pcr_event_hdr.digests;
	digests->count = 1;
	digests->digests[0].hash_alg = TPM2_ALG_SHA256;

	memset(&digests->digests[0].digest, 0, SHA256_SUM_LEN);

	rc = tpm_log_event(&pcr_event_hdr,
			   (const char *)&startup_locality_event);
	if (rc)
		pr_err("Log locality event failed with error %d\n", rc);

	return rc;
}
