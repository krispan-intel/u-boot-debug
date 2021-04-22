/*
 * Copyright (c) 2020, Intel Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <xlink_security.h>
#include <mapmem.h>
#include <stdlib.h>

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
 * Add xlink_secure.disable arguments to 'bootargs' env variable.
 *
 * Existing xlink_secure.disable arguments, if present, are removed and
 * the new ones are added.
 *
 * @param enabled The required status of xlink_secure_disable.
 *
 * @return 0 on success, 1 otherwise.
 */
static int xlink_security_add_boot_args(bool enabled)
{
	const char *cur_args;
	char *new_args, *ptr;
	int retv;

	/* Get current boot args. */
	cur_args = env_get("bootargs");
	if (!cur_args) {
		cur_args = "";
	}
	/* Create new bootargs. */
	new_args = calloc(strlen(cur_args) +
		sizeof(" xlink-secure.disable=x") + 1, sizeof(char));
	if (!new_args) {
		printf("Failed allocate memory for xlink_secure_disable bootargs\n");
		return 1;
	}
	/* Copy existing bootargs to new bootargs. */
	strcpy(new_args, cur_args);
	/* Remove the enablement arg ('xlink_secure.disable'). */
	remove_arg(new_args, "xlink-secure.disable=");
	/* Append new verity bootargs. */
	ptr = new_args + strlen(new_args);
	if (enabled) {
		sprintf(ptr, " xlink-secure.disable=1");
	} else {                /* Not enabled or roothash not present. */
		sprintf(ptr, " xlink-secure.disable=0");
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
int xlink_security_setup_boot_args(void)
{

	bool XLINK_SECURE_DISABLE = env_get_ulong("XLINK_SECURE_DISABLE", 2, 0);
	/* If XLINK_SECURE_DISABLE == 1, secure_xlink_driver
	 * in kernel will use normal xlink-api's. */
	if (xlink_security_add_boot_args(XLINK_SECURE_DISABLE)) {
		printf("xlink_security_setup_boot_args failed\n");
		return 1;
	}

	return 0;
}

