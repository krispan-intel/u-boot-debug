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

#define MEM_TYPE_MAX	8
#define PRIME_CONFIG	4  /* (0,2),(0,3),(1,2),(1,3) */

#define PRIME_0_2_CFG	0
#define PRIME_0_3_CFG	1
#define PRIME_1_2_CFG	2
#define PRIME_1_3_CFG	3

/* DTB file names config based on BoardID & Fuse settings*/
#define THB_FULL_4GB_DTB_CONF		"thb_full_4gb-1"
#define THB_FULL_8GB_DTB_CONF		"thb_full_8gb-1"
#define THB_PRIME_0_2_4GB_DTB_CONF	"thb_prime_0_2_4gb-1"
#define THB_PRIME_0_2_8GB_DTB_CONF	"thb_prime_0_2_8gb-1"
#define THB_PRIME_0_3_4GB_DTB_CONF	"thb_prime_0_3_4gb-1"
#define THB_PRIME_0_3_8GB_DTB_CONF	"thb_prime_0_3_8gb-1"
#define THB_PRIME_1_2_4GB_DTB_CONF	"thb_prime_1_2_4gb-1"
#define THB_PRIME_1_2_8GB_DTB_CONF	"thb_prime_1_2_8gb-1"
#define THB_PRIME_1_3_4GB_DTB_CONF	"thb_prime_1_3_4gb-1"
#define THB_PRIME_1_3_8GB_DTB_CONF	"thb_prime_1_3_8gb-1"
#define THB_PRIME_CRB2_0_2_4GB_DTB_CONF	"thb_prime_crb2_0_2_4gb-1"
#define THB_PRIME_CRB2_0_3_4GB_DTB_CONF	"thb_prime_crb2_0_3_4gb-1"
#define THB_PRIME_CRB2_1_2_4GB_DTB_CONF	"thb_prime_crb2_1_2_4gb-1"
#define THB_PRIME_CRB2_1_3_4GB_DTB_CONF	"thb_prime_crb2_1_3_4gb-1"
#define THB_FULL_8GB_4GB_DTB_CONF	"thb_full_8gb_4gb-1"
#define THB_PRIME_0_2_8GB_4GB_DTB_CONF	"thb_prime_0_2_8gb_4gb-1"
#define THB_PRIME_0_3_8GB_4GB_DTB_CONF	"thb_prime_0_3_8gb_4gb-1"
#define THB_PRIME_1_2_8GB_4GB_DTB_CONF	"thb_prime_1_2_8gb_4gb-1"
#define THB_PRIME_1_3_8GB_4GB_DTB_CONF	"thb_prime_1_3_8gb_4gb-1"


#ifndef __ASSEMBLY__

typedef struct dtb_config {
	int mem_id;
	char *dtb;
} dtb_config_t;

void config_dtb_blob(void);
#endif


#endif /* _THB_ENV_H_ */

