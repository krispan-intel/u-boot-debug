/*
 *  Copyright (c) 2019 Intel Corporation.
 *
 *  SPDX-License-Identifier:	GPL-2.0
 */

#ifndef __IMAGE_SVN_H__
#define __IMAGE_SVN_H__

#include <common.h>

/*
 * Get the value of the current SVN number (using board-specific storage).
 *
 * @param[out] svn The variable where to store the value of the current SVN.
 *
 * @return 0 if success, non-zero otherwise.
 */
int board_svn_get(u64 *svn);

/*
 * Update the value of the current SVN number (using board-specific storage).
 *
 * @param[in] new_svn The new value of the SVN.
 *
 * @return 0 if success, non-zero otherwise.
 */
int board_svn_set(u64 new_svn);

#endif /* __IMAGE_SVN_H__ */

