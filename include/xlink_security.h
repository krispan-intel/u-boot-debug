/*
 * Copyright (c) 2020, Intel Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/types.h>

/**
 * Setup up xlink-security boot args.
 *
 * set boot args related to xlink-security (i.e., 'xlink-secure.disable=<bool>').
 *
 * If setenv XLINK_SECURE_DISABLE == 1 on u-boot command shell bootargs are updated with
 * "xlink-secure.disable=1"; otherwise if setenv XLINK_SECURE_DISABLE == 0 *or* not set *or*
 * autoboot, bootargs are updated with "xlink-secure.disable=0".
 *
 * @return 0 on success ('bootargs' successfully updated), 1 on failure (e.g.,
 *           some internal error prevented 'bootargs' to be updated).
 */
int xlink_security_setup_boot_args(void);
