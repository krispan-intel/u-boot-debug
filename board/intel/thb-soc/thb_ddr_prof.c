/*
 * Copyright (c) 2021, Intel Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
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
 * Add ddr_prof.soc_cfg argument to 'bootargs' env variable.
 *
 * Existing ddr_prof.soc_cfg arguments, if present, are removed
 * and the new ones are added.
 *
 * @param soc_cfg soc configuration,tbh full or
 *                tbh prime combinations
` *
 * @return 0 on success, 1 otherwise.
 */
int ddr_prof_add_boot_args(u8 tbh_cfg, u8 *slice_cfg)
{
	const char *cur_args;
	char *new_args, *ptr, *soc_cfg;
	int ret;

	/* Get current boot args. */
	cur_args = env_get("bootargs");

	if (!cur_args) {
		cur_args = "";
	}
	/* Create new bootargs. */
	new_args = calloc(strlen(cur_args) +
		sizeof(" ddr_prof.soc_cfg=x") + 1, sizeof(char));

	if (!new_args) {
		printf("Failed to allocate memory for ddr_prof soc_cfg bootargs\n");
		return 1;
	}

	/* Copy existing bootargs to new bootargs. */
	strcpy(new_args, cur_args);
	/* Remove the enablement arg ('ddr_prof.soc_cfg') */
	remove_arg(new_args, "ddr_prof.soc_cfg=");
	/* Append new ddr_prof bootargs. */
	ptr = new_args + strlen(new_args);
	if (tbh_cfg)
		soc_cfg = " ddr_prof.soc_cfg=1";
	else if (slice_cfg[0] & slice_cfg[2])
		soc_cfg = " ddr_prof.soc_cfg=2";
	else if (slice_cfg[0] & slice_cfg[3])
		soc_cfg = " ddr_prof.soc_cfg=3";
	else if (slice_cfg[1] & slice_cfg[2])
		soc_cfg = " ddr_prof.soc_cfg=4";
	else if (slice_cfg[1] & slice_cfg[3])
		soc_cfg = " ddr_prof.soc_cfg=5";

	sprintf(ptr, soc_cfg);
	/* Update bootargs. */
	ret = env_set("bootargs", new_args);
	if (ret) {
		printf("Failed to update 'bootargs' env variable\n");
	}
	free(new_args);
	return ret;
}
