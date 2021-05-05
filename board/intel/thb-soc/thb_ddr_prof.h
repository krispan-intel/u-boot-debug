/*
 * Copyright (c) 2021, Intel Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/types.h>

/**
 * Set up ddr_prof boot args (i.e., 'ddr_prof.soc_cfg=<int>').
 *
 * Update ddr_prof.soc_cfg in boot args based on soc configuration.
 *
 * ddr_prof.soc_cfg = 1, TBH FULL
 * ddr_prof.soc_cfg = 2, TBH PRIME slice 0 and 2 enabled
 * ddr_prof.soc_cfg = 3, TBH PRIME slice 0 and 3 enabled
 * ddr_prof.soc_cfg = 4, TBH PRIME slice 1 and 2 enabled
 * ddr_prof.soc_cfg = 5, TBH PRIME slice 1 and 3 enabled
 *
 * @return 0 on success ('bootargs' successfully updated), 1 on failure (e.g.,
 *           some internal error prevented 'bootargs' to be updated).
 */
int ddr_prof_setup_boot_args(void);
int ddr_prof_add_boot_args(u8 tbh_cfg, u8 *slice_cfg);
