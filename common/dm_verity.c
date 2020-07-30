/*
 * Copyright (c) 2018, Intel Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm_verity.h>
#include <mapmem.h>
#include <stdlib.h>

/*
 * TODO: change this to a function checking the value of the security fuse bits
 * (once implemented). For the moment we use the value of the SECURE_SKU env
 * variable; if the variable is not set, default value ('1') is returned.
 */
#define SECURE_SKU (env_get_ulong("SECURE_SKU", 2, 1))

/** The size of the root hash. */
#define VERITY_HASH_SIZE_BYTES (48)
/** Size of the verity blob (magic + hash + padding) */
#define VERITY_BLOB_SIZE (8 + VERITY_HASH_SIZE_BYTES + 8)
/** The size of the root hash as a string. */
#define VERITY_HASH_SIZE_STR   (VERITY_HASH_SIZE_BYTES * 2 + 1)

/** The 'magic' for the dm-verity blob (value = 'DMVerity'). */
static const u8 magic[] = { 0x44, 0x4d, 0x56, 0x65, 0x72, 0x69, 0x74, 0x79 };

/**
 * Get the verity root hash appended to a Linux aarch64 Image.
 *
 * @param[in]  image_addr   The address where the image is stored.
 * @param[in]  image_size   The size of the image as reported in the Image
 *			    header.
 * @param[out] hash_str     The buffer where to store the hash (as a string).
 *			    Must not be NULL. If the image has no hash, then
 *			    an empty string is stored in the buffer.
 * @param[in]  hash_str_len The size of the buffer.
 *
 * @return 0 on success, 1 in case of error (e.g., the output buffer is too
 *	   small). Note: the case of no hash being present is not considered an
 *	   error condition, so success is returned in that case as well.
 */
static int verity_get_hash_from_image(ulong image_addr, ulong image_size,
				      char *hash_str, ulong hash_str_len)
{
	int i;
	u8 *image, *hash_bin;

	if (!hash_str) {
		return 1;
	}
	if (hash_str_len < VERITY_HASH_SIZE_STR) {
		return 1;
	}
	image = (uint8_t *)map_sysmem(image_addr, 0);
	debug("image: %p\n", image);
	debug("image_size: %ld\n", image_size);

	hash_bin = NULL;
	/*
	 * Find verity hash in image; if not found, set output string to empty
	 * string. Return success in both cases, unless an error occurs.
	 */
	for (i = image_size - VERITY_BLOB_SIZE; i >= 0; i--) {
		if (memcmp(&image[i], magic, sizeof(magic)) == 0) {
			debug("Verity magic found @%p\n", &image[i]);
			hash_bin = &image[i + sizeof(magic)];
			break;
		}
	}
	if (hash_bin) {
		for (i = 0; i < VERITY_HASH_SIZE_BYTES; i++)
			sprintf(&hash_str[i * 2], "%02X", hash_bin[i]);
	} else {
		debug("Verity magic not found\n");
		*hash_str = '\0';
	}

	unmap_sysmem(image);
	return 0;
}

/**
 * Remove an argument (and its value) from a list of kernel arguments.
 *
 * @param args     The list of kernel arguments, as a string.
 * @param argument The argument to be removed. Must not be NULL and MUST
 *		   include the trailing '=' character.
 *
 * @return The number of occurrences of the specified argument found.
 */
static int remove_arg(char *args, const char *argument)
{
	char *arg_start, *arg_end;
	int retv = 0;

	if (!args || !argument) {
		return 1;
	}
	arg_start = args;
	while (arg_start) {
		/* Remove leading white spaces. */
		while (*arg_start == ' ')
			arg_start++;
		/*
		 * Check if the current argument matches the one to be removed.
		 */
		if (strstr(arg_start, argument) == arg_start) {
			/*
			 * We found an argument to be removed; now find where
			 * the argument ends (i.e, find the next space).
			 */
			arg_end = strchr(arg_start, ' ');
			/*
			 * If no space char found, then the argument is the
			 * last one; we are done, just truncate the string and
			 * return.
			 */
			if (!arg_end) {
				debug("Removing argument %s\n", arg_start);
				*arg_start = '\0';
				retv++;
				return retv;
			}
			/*
			 * Shift (copy) the remaining argument over the current
			 * one.
			 */
			debug("Removing argument %*s\n",
			      (int)(arg_end - arg_start), arg_start);
			memmove(arg_start, arg_end, strlen(arg_end) + 1);
			retv++;
			/*
			 * arg_start now contains the next arg, continue to
			 * check if it must be removed as well.
			 */
			continue;
		}
		/*
		 * Current argument is not to be removed, jump to the next one.
		 */
		arg_start = strchr(arg_start, ' ');
	}

	return retv;
}

/**
 * Add verity arguments to 'bootargs' env variable.
 *
 * Existing verity arguments, if present, are removed and the new ones are
 * added.
 *
 * @param enabled The required status of verity.
 * @param hash	  The value of the roothash parameter. If NULL or empty, verity
 *		  is disabled (overriding the value of the 'enabled' argument).
 *
 * @return 0 on success, 1 otherwise.
 */
static int verity_add_boot_args(bool enabled, const char *hash)
{
	const char *cur_args;
	char *new_args, *ptr;
	int retv;

	if (!hash) {
		hash = "";
	}
	/* Get current boot args. */
	cur_args = env_get("bootargs");
	if (!cur_args) {
		cur_args = "";
	}
	/* Create new bootargs. */
	new_args = calloc(strlen(cur_args) + sizeof(" verity=x")
			  + sizeof(" roothash=") + strlen(hash) + 1,
			  sizeof(char));
	if (!new_args) {
		printf("Failed to allocate memory for new verity bootargs\n");
		return 1;
	}
	/* Copy existing bootargs to new bootargs. */
	strcpy(new_args, cur_args);
	/* Remove the enablement arg ('verity') and the roothash arg. */
	remove_arg(new_args, "verity=");
	remove_arg(new_args, "roothash=");
	/* Append new verity bootargs. */
	ptr = new_args + strlen(new_args);
	if (enabled && *hash != '\0') {
		sprintf(ptr, " verity=1 roothash=%s", hash);
	} else {                /* Not enabled or roothash not present. */
		sprintf(ptr, " verity=0");
	}
	/* Update bootargs. */
	retv = env_set("bootargs", new_args);
	if (retv) {
		printf("Failed to update 'bootargs' env variable\n");
	}

	free(new_args);

	return retv;
}

/* See header file for documentation. */
int verity_setup_boot_args(ulong image, ulong image_size, bool full_scan)
{
	char *hash = NULL;

	/* If SECURE_SKU == 1, we must look for the roothash in the image. */
	if (SECURE_SKU) {
		if (!full_scan) {
			/*
			 * If 'full scan' is disabled, we look for the
			 * dm-verity blob only at the very end of the image. We
			 * do so by overriding the image start address and the
			 * image size.
			 */
			image += image_size - VERITY_BLOB_SIZE;
			image_size = VERITY_BLOB_SIZE;
		}
		hash = malloc(VERITY_HASH_SIZE_STR);
		if (!hash || verity_get_hash_from_image(image, image_size, hash,
							VERITY_HASH_SIZE_STR)) {
			free(hash);
			return 1;
		}
	}
	if (verity_add_boot_args(SECURE_SKU, hash)) {
		free(hash);
		return 1;
	}
	free(hash);

	return 0;
}

