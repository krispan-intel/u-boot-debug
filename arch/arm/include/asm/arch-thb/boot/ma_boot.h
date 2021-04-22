/*
 * Copyright (c) 2019-2020 Intel Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 * Original ATF 'ma_boot.h' file, after removing unneeded information.
 */

#ifndef __MA_BOOT_H__
#define __MA_BOOT_H__

/**
 * Boot Types.
 */
typedef enum {
	MA_BOOT_GPIO = 0,
	MA_BOOT_EFUSE = 1
} ma_boot_type_t;

/**
 * Boot qualifier settings.
 */
typedef enum {
	/* DEBUG boot Qual settings. */
	MA_QUAL_DEBUG_PLL_OFF = 0,
	MA_QUAL_DEBUG_PLL_RANGE_LO = 1,
	MA_QUAL_DEBUG_PLL_RANGE_MID = 2,
	MA_QUAL_DEBUG_PLL_RANGE_HI = 3,
	/* eMMC boot Qual settings. */
	MA_QUAL_EMMC_PCIE_NO_INIT = 0,
	MA_QUAL_EMMC_PCIE_INIT_0 = 1,
	MA_QUAL_EMMC_PCIE_INIT_1 = 2,
	MA_QUAL_EMMC_PCIE_INIT_2 = 3,
	/* SPI boot Qual settings. */
	MA_QUAL_SPI_BOOT_SIZE_4MB = 0,
	MA_QUAL_SPI_BOOT_SIZE_1MB = 1,
	MA_QUAL_SPI_BOOT_SIZE_2MB = 2,
	MA_QUAL_SPI_BOOT_SIZE_512KB = 3
} ma_boot_qual_t;

#endif /* __MA_BOOT_H__ */
