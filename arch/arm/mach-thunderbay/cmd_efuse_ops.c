// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2020 Intel Corporation
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <asm/arch/sip_svc.h>
#include <asm/arch/thb_shared_mem.h>
#include <asm/arch/boot/thb-efuse.h>

#define FUSE_BLOB_LOAD_BASE 0x101A000000

/**
 * eFuse provisioning status.
 */
typedef enum {
        /** efuses not provisioned. */
        MA_EFUSE_NOT_PROVISIONED = 1,
        /** efuses provisioned. */
        MA_EFUSE_ALREADY_PROVISIONED,
        /** Unable to determine provisioning status. */
        MA_EFUSE_PROVISIONING_UNKNOWN,
} ma_efuse_provisioning_status_t;

/**
 * Provisioning status return
 * codes to BL31
 */
typedef enum{
        MA_EFUSE_PROVISION_SUCCESS = 4,
        MA_EFUSE_PROVISION_SUCCESS_COLD_RESET_REQUIRED,
        MA_EFUSE_PROVISION_DATA_INVALID,
        MA_EFUSE_PROVISION_DATA_NULL,
        MA_EFUSE_PROVISION_FAILED,
}ma_efuse_blprovision_stat_t;


#define EFUSE_SIP_READ_ROWS_FAIL        -101
#define EFUSE_SIP_WRITE_ROWS_FAIL       -102


static void efuse_error_status(int rc)
{
	switch (rc) {
	case -EFAULT:
		pr_err("User buffer not in SHMEM.\n");
		break;
	case -EINVAL:
		pr_err("Input parameter is not correct.\n");
		break;
	case -EEXIST:
		pr_err("Fuse operation failed, hardware used.\n");
		break;
	case -EACCES:
		pr_err("Fuse operation failed, hardware locked.\n");
		break;
	case MA_EFUSE_PROVISION_DATA_INVALID:
		pr_err("eFuse platform data validation failed.\n");
		break;
	case MA_EFUSE_PROVISION_DATA_NULL:
		pr_err("Couldn't find the eFuse Provision Data.\n");
		break;
	case EFUSE_SIP_WRITE_ROWS_FAIL:
		pr_err("Fuse operation failed, Write Rows failed.\n");
		break;
	case EFUSE_SIP_READ_ROWS_FAIL:
		pr_err("Fuse operation failed, Read Rows Failed.\n");
		break;
	case MA_EFUSE_ALREADY_PROVISIONED:
		pr_err("eFuses already provisioned or provisioning not possible\n");
		break;
	case MA_EFUSE_PROVISIONING_UNKNOWN:
		pr_err("Unable to determine provisioning status.\n");
		break;
	case MA_EFUSE_PROVISION_FAILED:
		pr_err("efuses provisioning Failed.\n");
		break;
	default:
		pr_err("Unexpected Error occurred with error code = %d\n", rc);
	}
}

int efuse_read_ranges(u32 * const boot_operation,
			     const u32 start_u32_idx,
			     const u32 end_u32_idx)
{
	u32 i;
	int rc;

	rc = sip_svc_efuse_read_ranges((const uintptr_t)boot_operation,
				       (const u32)start_u32_idx,
				       (const u32)end_u32_idx);

	if (rc != 0) {
		efuse_error_status(rc);
		return CMD_RET_FAILURE;
	}

	for (i = start_u32_idx; i <= end_u32_idx; i++)
		printf("The fuse value at idx %d = 0x%x\n", i,
		       boot_operation[i - start_u32_idx]);

	return CMD_RET_SUCCESS;
}

int efuse_write_ranges(u32 * const boot_operation,
			      u32 * const fuse_mask, const u32 start_u32_idx,
			      const u32 end_u32_idx, const u32 flags)
{
	int rc;
	u32 i = 0;

	rc = sip_svc_efuse_write_ranges((const uintptr_t)boot_operation,
					(const uintptr_t)fuse_mask,
					(const uint32_t)start_u32_idx,
					(const uint32_t)end_u32_idx,
					(enum soc_efuse_write_flags)flags);

	if (rc != 0) {
		efuse_error_status(rc);
		return CMD_RET_FAILURE;
	}

	if (flags & SOC_EFUSE_FLAG_READBACK) {
		for (i = start_u32_idx; i <= end_u32_idx; i++)
			printf("The fuse value at idx %d = 0x%x\n", i,
			       boot_operation[i - start_u32_idx]);
	}

	return CMD_RET_SUCCESS;
}

static int do_efuse(struct cmd_tbl *cmdtp, int flag, int argc,
		    char *const argv[])
{
	u32 start_idx, end_idx, flags;
	char *mask_field_str, *saveptr;
	u32 *fuse_mask, *boot_operation;
	u32 i = 0;
	u64 blob_address = 0;
	u32 blob_size = 0;
	u32 num_fuse_entries, prov_stat;
	int rc;

	if (argc < 2)
		return CMD_RET_USAGE;

	if ((!strcmp(argv[1], "read_range_efuses")) && argc == 4) {
		start_idx = (u32)simple_strtoul(argv[2], NULL, 10);
		end_idx = (u32)simple_strtoul(argv[3], NULL, 10);

		if (start_idx >= MA_EFUSE_NUM_BITS / 64) {
			efuse_error_status(-EINVAL);
			return CMD_RET_FAILURE;
		}

		if (end_idx >= MA_EFUSE_NUM_BITS / 64) {
			efuse_error_status(-EINVAL);
			return CMD_RET_FAILURE;
		}

		if (end_idx < start_idx) {
			efuse_error_status(-EINVAL);
			return CMD_RET_FAILURE;
		}

		num_fuse_entries = (end_idx - start_idx) + 1;

		boot_operation = (u32 *)get_secure_shmem_ptr((sizeof(u32) * num_fuse_entries));

		if (!boot_operation) {
			log_err("Unable to retrieve shared memory.\n");
			return CMD_RET_FAILURE;
		}

		return efuse_read_ranges(boot_operation, start_idx, end_idx);
	}

	if ((!strcmp(argv[1], "write_range_efuses")) && argc == 6) {
		start_idx = (u32)simple_strtoul(argv[2], NULL, 10);
		end_idx = (u32)simple_strtoul(argv[3], NULL, 10);
		flags = (u32)simple_strtoul(argv[5], NULL, 16);

		if (start_idx >= MA_EFUSE_NUM_BITS / 64) {
			efuse_error_status(-EINVAL);
			return CMD_RET_FAILURE;
		}

		if (end_idx >= MA_EFUSE_NUM_BITS / 64) {
			efuse_error_status(-EINVAL);
			return CMD_RET_FAILURE;
		}

		if (end_idx < start_idx) {
			efuse_error_status(-EINVAL);
			return CMD_RET_FAILURE;
		}

		num_fuse_entries = (end_idx - start_idx) + 1;

		/* Request shared memory for both boot operation and fuse mask.
		 * fuse_mask has to be same size as number of fuse entries
		 * to be programmed.
		 */
		boot_operation = (u32 *)get_secure_shmem_ptr(2 * (sizeof(u32) * num_fuse_entries));

		if (!boot_operation) {
			log_err("Unable to retrieve shared memory.\n");
			return CMD_RET_FAILURE;
		}

		fuse_mask = boot_operation + (sizeof(u32) * num_fuse_entries);

		saveptr = argv[4];
		mask_field_str = strsep(&saveptr, ",");
		/* Split the mask_field string and store into array.*/
		while (mask_field_str &&
		       (i <= (end_idx - start_idx))) {
			fuse_mask[i] = (u32)simple_strtoul(mask_field_str,
			 NULL, 16);
			mask_field_str = strsep(&saveptr, ",");
			i++;
		}

		/* Size of mask buffer should be equal to no. of fuses
		 * to be provisioned.
		 */
		if (i != ((end_idx - start_idx) + 1)) {
			pr_err("Invalid number of mask parameters given.\n");
			return CMD_RET_FAILURE;
		}

		return efuse_write_ranges(boot_operation, fuse_mask, start_idx,
				end_idx, flags);
	}

	if ((!strcmp(argv[1], "provision")) && argc == 4) {
		blob_address = (u64)simple_strtoul(argv[2], NULL, 16);
		blob_size = (u32)simple_strtoul(argv[3], NULL, 16);

		if (blob_address < FUSE_BLOB_LOAD_BASE || (blob_address >
			(FUSE_BLOB_LOAD_BASE + EFUSE_PDATA_MAX_SIZE))) {
			efuse_error_status(-EINVAL);
			return CMD_RET_FAILURE;
		}

		if (blob_size > EFUSE_PDATA_MAX_SIZE) {
			efuse_error_status(-EINVAL);
			return CMD_RET_FAILURE;
		}

		boot_operation = (u32 *)get_secure_shmem_ptr(sizeof(uint8_t)*blob_size);

		if (!boot_operation) {
			log_err("Unable to allocate shared memory for fuse blob.\n");
			return CMD_RET_FAILURE;
		}

		memcpy(boot_operation, blob_address, blob_size);

		/* SMC the address and size of the blob. */
		rc = sip_svc_efuse_provision((const uintptr_t)boot_operation,
					     (const size_t)blob_size);

		if (rc == MA_EFUSE_PROVISION_SUCCESS_COLD_RESET_REQUIRED) {
			printf("Perform a cold reset for fuses to take effect\n");
			return CMD_RET_SUCCESS;
		}

		if (rc != 0) {
			efuse_error_status(rc);
			return CMD_RET_FAILURE;
		}

		printf("Provisioning successful\n");

		return CMD_RET_SUCCESS;
	}

	if ((!strcmp(argv[1], "status")) && argc == 2) {

		prov_stat = sip_svc_efuse_status();

		switch(prov_stat)
		{
			case MA_EFUSE_NOT_PROVISIONED:
				pr_info("Provision Status : Not Provisioned\n");
				break;
			case MA_EFUSE_ALREADY_PROVISIONED:
				pr_info("Provision Status : Provisioned\n");
				break;
			case MA_EFUSE_PROVISIONING_UNKNOWN:
				pr_info("Provision Status : Unknown\n");
				break;
			default:
				pr_info("Provision Status : Unknown\n");
		}
		return CMD_RET_SUCCESS;
	}

	/* Show command usage if none of above */
	return CMD_RET_USAGE;
}

U_BOOT_CMD(thunderbay_efuse, 6, 0, do_efuse, "Perform Efuse operations\n",
	   "read_range_efuses <start_index> <end_index>\n"
	   "thunderbay_efuse write_range_efuses <start_index> <end_index> <mask1,mask2,..> <flag>\n"
	   "thunderbay_efuse provision <address> <sizeof blob>\n"
	   "thunderbay_efuse status\n"
);
