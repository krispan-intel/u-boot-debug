/*
 *  Copyright (c) 2019 Intel Corporation.
 *
 *  SPDX-License-Identifier:	GPL-2.0
 */

#include <image-svn.h>

#include <stdio.h>
#include <env.h>
#include <image.h>

/*
 * SECURE_SKU env variable is updated to 1 when security fuse
 * bits are set; if the variable is not set, default value ('0')
 * is returned.
 */
#define SECURE_SKU (env_get_ulong("SECURE_SKU", 2, 0))

/*TODO: remove this to get the svn from board. Currently SVN is getting from u-boot env */
//#define CONFIG_FIT_SVN_STORAGE_ENV 1

/* The name of the SVN node in the FIT image. */
#define FIT_SVN_NODENAME "svn"
#define SVN_MAX_NUM      128

#if defined(CONFIG_FIT_SVN_STORAGE_ENV)
/* Use U-Boot environment for storing SVN. */
#define SVN_ENV_VAR "SVN"
#define svn_get(svn) env_svn_get(svn)
#define svn_set(svn) env_svn_set(svn)
/*
 * Get the value of the current SVN number (stored in the U-Boot env).
 *
 * @param[out] svn The variable where to store the value of the current SVN.
 *
 * @return 0 (function cannot fail).
 */
static int env_svn_get(u32 *svn)
{
	/*
	 * Set output parameter to the value of "SVN" env variable, or 0 if not
	 * defined.
	 */
	*svn = env_get_ulong(SVN_ENV_VAR, 0, 0);

	return 0;
}

/*
 * Update the value of the current SVN number (stored in the U-Boot env).
 *
 * @param[in] new_svn The new value of the SVN.
 *
 * @return 0 if success, non-zero otherwise.
 */
static int env_svn_set(u32 new_svn)
{
	int retv;

	retv = env_set_ulong(SVN_ENV_VAR, new_svn);
	if (retv) {
		return retv;
	}
	retv = env_save();

	return retv;
}
#elif defined(CONFIG_FIT_SVN_STORAGE_BOARD_SPECIFIC)
/* Use board-specific functions for storing and retrieving SVN. */
#define svn_get(svn) board_svn_get(svn)
#define svn_set(svn) board_svn_set(svn)
#elif defined(CONFIG_THUNDERBAY_FIT_SVN_STORAGE)
#define svn_get(svn) tbh_os_svn_get(svn)
#define svn_set(svn) tbh_os_svn_set(svn)
#endif /* CONFIG_FIT_SVN_STORAGE_ENV */

int fit_config_check_svn(const void *fit, int conf_noffset)
{
	int lenp, retv;
	u32 svn_img, svn_cur;
	const struct fdt_property *prop;

	/* If secure SKU is disabled, do not enforce SVN check. */
	if (!SECURE_SKU) {
		return 0;
	}
	/* Get current SVN. */
	retv = svn_get(&svn_cur);
	if (retv) {
		return retv;
	}
	/* Get image SVN. */
	if (!fit) {
		return -EINVAL;
	}
	prop = fdt_get_property(fit, conf_noffset, FIT_SVN_NODENAME, &lenp);
	if (!prop) {
		return -ENOENT;
	}
	if (lenp != sizeof(u32)) {
		return -EILSEQ;
	}
	svn_img = fdt32_to_cpu(*(fdt32_t *)prop->data);

	if (svn_img > SVN_MAX_NUM)
		return -EINVAL;

	/* Compare image SVN with current SVN. */
	if (svn_img < svn_cur) {
		return -EPERM;
	}
	if (svn_img > svn_cur) {
		retv = svn_set(svn_img);
		/* Try to update SVN. */
		if (retv) {
			return retv;
		}
	}
	return 0;
}

