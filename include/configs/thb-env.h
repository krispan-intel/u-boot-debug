/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020, Intel Corporation
 */

#ifndef _THB_ENV_H_
#define _THB_ENV_H_

/*
 * Environment configuration.
 */
#define THB_SYS_MMC_ENV_DEV 0
#define THB_ENV_OFFSET (0x2100000)
#define THB_ENV_SIZE (0x80000)
#define THB_ENV_OFFSET_REDUND (THB_ENV_OFFSET + THB_ENV_SIZE)
/* Env size and offset as strings (used to define Thunderbay Bay GPT) */
#define THB_ENV_OFFSET_STR "33MB"
#define THB_ENV_SIZE_STR "512KB"

/* DTB file names config based on BoardID & Fuse settings*/
#define THB_FULL_4GB_DTB_CONF		"thb_full_4gb@1"
#define THB_FULL_8GB_DTB_CONF		"thb_full_8gb@1"
#define THB_PRIME_0_2_4GB_DTB_CONF	"thb_prime_0_2_4gb@1"
#define THB_PRIME_0_2_8GB_DTB_CONF	"thb_prime_0_2_8gb@1"
#define THB_PRIME_0_3_4GB_DTB_CONF	"thb_prime_0_3_4gb@1"
#define THB_PRIME_0_3_8GB_DTB_CONF	"thb_prime_0_3_8gb@1"
#define THB_PRIME_1_2_4GB_DTB_CONF	"thb_prime_1_2_4gb@1"
#define THB_PRIME_1_2_8GB_DTB_CONF	"thb_prime_1_2_8gb@1"
#define THB_PRIME_1_3_4GB_DTB_CONF	"thb_prime_1_3_4gb@1"
#define THB_PRIME_1_3_8GB_DTB_CONF	"thb_prime_1_3_8gb@1"
#define THB_PRIME_CRB2_0_2_4GB_DTB_CONF	"thb_prime_crb2_0_2_4gb@1"
#define THB_PRIME_CRB2_0_3_4GB_DTB_CONF	"thb_prime_crb2_0_3_4gb@1"
#define THB_PRIME_CRB2_1_2_4GB_DTB_CONF	"thb_prime_crb2_1_2_4gb@1"
#define THB_PRIME_CRB2_1_3_4GB_DTB_CONF	"thb_prime_crb2_1_3_4gb@1"


#ifndef __ASSEMBLY__
void config_dtb_blob(void);
#endif


#endif /* _THB_ENV_H_ */

