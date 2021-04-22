/*
 *Copyright (c) 2019-2020 Intel Corporation.
 *
 * For SD/eMMC PHY configuration.
 */

#ifndef _ARCH_THB_SD_EMMC_PHY_H_
#define _ARCH_THB_SD_EMMC_PHY_H_

#include <common.h>
#include <asm/io.h>
#include <mmc.h>


/* Describes the PHY register set. */
struct thunderbay_sd_mmc_phy_regs {
	uint32_t ctrl_cfg_0;
	uint32_t ctrl_cfg_1;
	uint32_t ctrl_preset_0;
	uint32_t ctrl_preset_1;
	uint32_t ctrl_preset_2;
	uint32_t ctrl_preset_3;
	uint32_t ctrl_preset_4;
	uint32_t ctrl_cfg_2;
	uint32_t ctrl_cfg_3;
	uint32_t phy_cfg_0;
	uint32_t phy_cfg_1;
	uint32_t phy_cfg_2;
	uint32_t phybist_ctrl;
	uint32_t sdhc_stat_3;
	uint32_t phy_stat;
};

/* Bit position of base clock frequency in CTRL_CFG_0 register. */
#define CTRL_CFG_0_BASE_CLK_FREQ_OFFSET (14)
/* Bit mask of base clock frequency in CTRL_CFG_0 register. */
#define CTRL_CFG_0_BASE_CLK_FREQ_MASK (0xFF)

/* Bit position of HS support in CTRL_CFG_0 register. */
#define CTRL_CFG_0_SUPPORT_HS_OFFSET (26)
/* Bit mask of all speed caps in CTRL_CFG_0 register. */
#define CTRL_CFG_0_SPEED_CAPS_MASK (BIT(CTRL_CFG_0_SUPPORT_HS_OFFSET))

/* Bit position of SUPPORT_64B support in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SUPPORT_64B_OFFSET (24)
/* Bit position of SLOT_TYPE support in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SLOT_TYPE_OFFSET (26)
/* Bit mask of SLOT_TYPE in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SLOT_TYPE_MASK (0x3)
/* Value of 'Removable' in SLOT_TYPE in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SLOT_TYPE_REMOVABLE (0x0)

/* Bit position of SDR50 support in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SUPPORT_SDR50_OFFSET (28)
/* Bit position of DDR50 support in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SUPPORT_DDR50_OFFSET (1)
/* Bit position of SDR104 support in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SUPPORT_SDR104_OFFSET (0)
/* Bit position of HS400 support in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SUPPORT_HS400_OFFSET (2)
/* Bit mask of all speed caps in CTRL_CFG_1 register. */
#define CTRL_CFG_1_SPEED_CAPS_MASK                                             \
	(BIT(CTRL_CFG_1_SUPPORT_SDR50_OFFSET) |                                \
	 BIT(CTRL_CFG_1_SUPPORT_DDR50_OFFSET) |                                \
	 BIT(CTRL_CFG_1_SUPPORT_SDR104_OFFSET) |                               \
	 BIT(CTRL_CFG_1_SUPPORT_HS400_OFFSET))

/* Bit mask of OTAP_DLY_ENA in PHY_CFG_0 register. */
#define PHY_CFG_0_OTAP_DLY_ENA_MASK (0x1)
/* Bit mask of OTAP_DLY_SEL in PHY_CFG_0 register. */
#define PHY_CFG_0_OTAP_DLY_SEL_MASK GENMASK(3, 0)
/* Number of bits of OTAP_DLY_SEL in PHY_CFG_0 register. */
#define PHY_CFG_0_OTAP_DLY_SEL_BITS (4)
/* Bit mask of ITAP_DLY_ENA in PHY_CFG_0 register. */
#define PHY_CFG_0_ITAP_DLY_ENA_MASK (0x1)
/* Bit mask of ITAP_DLY_SEL in PHY_CFG_0 register. */
#define PHY_CFG_0_ITAP_DLY_SEL_MASK GENMASK(4, 0)
/* Number of bits of ITAP_DLY_SEL in PHY_CFG_0 register. */
#define PHY_CFG_0_ITAP_DLY_SEL_BITS (5)
/* Bit position of ITAP_DLY_x in PHY_CFG_0 register. */
#define PHY_CFG_0_ITAP_DLY_X_OFFSET (16)
/* Bit mask of ITAP_DLY_x in PHY_CFG_0 register. */
#define PHY_CFG_0_ITAP_DLY_X_MASK GENMASK(21, 16)
/* Bit position of OTAP_DLY_x in PHY_CFG_0 register. */
#define PHY_CFG_0_OTAP_DLY_X_OFFSET (23)
/* Bit mask of OTAP_DLY_x in PHY_CFG_0 register. */
#define PHY_CFG_0_OTAP_DLY_X_MASK GENMASK(27, 23)
/* Bit position of DLL_EN in PHY_CFG_0 register. */
#define PHY_CFG_0_DLL_EN_OFFSET (10)
/* Bit mask of DLL_EN in PHY_CFG_0 register. */
#define PHY_CFG_0_DLL_EN_MASK BIT(PHY_CFG_0_DLL_EN_OFFSET)
/* Bit mask of SEL_DLY_RXCLK in PHY_CFG_0 register. */
#define PHY_CFG_0_SEL_DLY_RXCLK_MASK BIT(28)
/* Bit position of SEL_DLY_RXCLK in PHY_CFG_0 register. */
#define PHY_CFG_0_SEL_DLY_RXCLK_OFFSET (28)
/* Bit mask of SEL_DLY_TXCLK in PHY_CFG_0 register. */
#define PHY_CFG_0_SEL_DLY_TXCLK_MASK BIT(29)
/* Bit position of SEL_DLY_TXCLK in PHY_CFG_0 register. */
#define PHY_CFG_0_SEL_DLY_TXCLK_OFFSET (29)

/* Bit position of PWR_DOWN in PHY_CFG_0 register. */
#define PHY_CFG_0_PWR_DOWN_OFFSET (0)
/* Bit mask of PWR_DOWN in PHY_CFG_0 register. */
#define PHY_CFG_0_PWR_DOWN_MASK (0x1)
/* Value of power enable for PWR_DOWN in PHY_CFG_0 register */
#define PHY_CFG_0_PWR_DOWN_EN (0x1)

/* Bit position of ITAP_CHG_WIN in PHY_CFG_0 register. */
#define PHY_CFG_0_ITAP_CHG_WIN_OFFSET (22)
/* Bit mask of ITAP_CHG_WIN in PHY_CFG_0 register. */
#define PHY_CFG_0_ITAP_CHG_WIN_MASK (BIT(PHY_CFG_0_ITAP_CHG_WIN_OFFSET))

/* Bit position of REN_x in PHY_CFG_1 register. */
#define PHY_CFG_1_REN_X_OFFSET (10)
/* Bit mask of REN_x in PHY_CFG_1 register. */
#define PHY_CFG_1_REN_X_MASK GENMASK(19, 10)

/* Bit position of SEL_FREQ in PHY_CFG_2 register. */
#define PHY_CFG_2_SEL_FREQ_OFFSET (10)
/* Bit mask of SEL_FREQ in PHY_CFG_2 register. */
#define PHY_CFG_2_SEL_FREQ_MASK GENMASK(12, 10)

/* Bit position of SEL_STRB in PHY_CFG_2 register. */
#define PHY_CFG_2_SEL_STRB_OFFSET (13)
/* Bit mask of SEL_STRB in PHY_CFG_2 register. */
#define PHY_CFG_2_SEL_STRB_MASK GENMASK(16, 13)

/* Value indicating '200 - 170' MHz' for SEL_FREQ in PHY_CFG_2 register */
#define PHY_CFG_2_SEL_FREQ_200_170 (0)
/* Value indicating '170 - 140' MHz' for SEL_FREQ in PHY_CFG_2 register */
#define PHY_CFG_2_SEL_FREQ_170_140 (1)
/* Value indicating '140 - 110' MHz' for SEL_FREQ in PHY_CFG_2 register */
#define PHY_CFG_2_SEL_FREQ_140_110 (2)
/* Value indicating '110 - 80' MHz' for SEL_FREQ in PHY_CFG_2 register */
#define PHY_CFG_2_SEL_FREQ_110_80 (3)
/* Value indicating '80 - 50' MHz' for SEL_FREQ in PHY_CFG_2 register */
#define PHY_CFG_2_SEL_FREQ_80_50 (4)
/* Value indicating '275 - 250' MHz' for SEL_FREQ in PHY_CFG_2 register */
#define PHY_CFG_2_SEL_FREQ_275_250 (5)
/* Value indicating '250 - 225' MHz' for SEL_FREQ in PHY_CFG_2 register */
#define PHY_CFG_2_SEL_FREQ_250_225 (6)
/* Value indicating '225 - 200' MHz' for SEL_FREQ in PHY_CFG_2 register */
#define PHY_CFG_2_SEL_FREQ_225_200 (7)

/* Bit offset of DLL_RDY in PHY_STAT register */
#define PHY_STAT_DLL_RDY_OFFSET (5)
/* Bit mask of DLL_RDY PHY_STAT register. */
#define PHY_STAT_DLL_RDY_MASK BIT(PHY_STAT_DLL_RDY_OFFSET)
/* Value indicating DLL ready in DLL_RDY for PHY_STAT register */

/**
 * board_arasan_sdhci_get_delay() - give SDHCI driver delay parameters
 * @mmc:		MMC for which parameters requested
 * @itap_dly_ena:	Pointer to where to store ITAP delay enable,
 * 			0 to disable 1 to enable
 * @itap_dly_sel:	Pointer to where to store ITAP delay, in range 0 to 15
 * @otap_dly_ena:	Pointer to where to store OTAP delay enable,
 * 			0 to disable 1 to enable
 * @otap_dly_sel:	Pointer to where to store OTAP delay, in range 0 to 15
 *
 * Board code must supply board-specific parameters for tuning delay.
 *
 * @return:		0 if the values have been updated, -1 if no values were
 *			updated, the delay in the hardware will not be updated.
 */
int board_thunderbay_sdhci_get_delay(struct mmc *mmc, u8 *itap_dly_ena,
				  u8 *itap_dly_sel, u8 *otap_dly_ena,
				  u8 *otap_dly_sel);

#endif /* _ARCH_THB_SD_EMMC_PHY_H_ */
