// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019-2020 Intel Corporation.
 */

#include <asm/arch-thb/sd_emmc_phy.h>
#include <clk.h>
#include <common.h>
#include <dm.h>
#include <memalign.h>
#include <sdhci.h>
#include <fdt_support.h>

/*
 * Thunder Bay SDHCI requires card detect to be true for SD card
 * before usage, but the time for card detect to go to 1 after
 * enabling the signal can be greater than 1 second.
 */
#define THUNDERBAY_SDHCI_CD_TIMEOUT_MS (2000)

/*
 * Host control 2 UHS bits
 */
#define THUNDERBAY_SDHCI_HOST_CTRL2_UHS_MASK GENMASK(2, 0)
#define THUNDERBAY_SDHCI_HOST_CTRL2_UHS_OFFSET (0)
#define THUNDERBAY_SDHCI_HOST_CTRL2_UHS_SDR12 (0)
#define THUNDERBAY_SDHCI_HOST_CTRL2_UHS_SDR25 (1)
#define THUNDERBAY_SDHCI_HOST_CTRL2_UHS_SDR50 (2)
#define THUNDERBAY_SDHCI_HOST_CTRL2_UHS_SDR104 (3)
#define THUNDERBAY_SDHCI_HOST_CTRL2_UHS_DDR50 (4)
#define THUNDERBAY_SDHCI_HOST_CTRL2_UHS_HS400 (5)

/*
 * These registers are 'reserved' in the typical SDHCI controller for U-Boot.
 * Adding defines for them here.
 */
#define SDHCI_HOST_CONTROL_2 0x3E

/* Register bits for 'host control 2' register */
#define SDHCI_CTRL2_SAMPLING_CLOCK_SELECT BIT(7)
#define SDHCI_CTRL2_EXECUTE_TUNING BIT(6)

/*
 * This is an invalid value for a PHY configuraiton bit in the eMMC
 * PHY. If we don't find any configuration in the device tree,
 * we just don't touch the relevant bits.
 */
#define THUNDERBAY_SDHCI_PHY_PROP_INVALID_VALUE (0xFFFFFFFF)

/* Tuning loops */
#define THUNDERBAY_SDHCI_TUNING_LOOPS 40

struct thunderbay_sdhci_plat {
	struct mmc_config config;
	struct mmc mmc;
};

struct thunderbay_sdhci_priv {
	struct sdhci_host host;
	unsigned long clk_in;
	unsigned long f_max;
	bool broken_cd;
	void __iomem *phy_addr;
	bool phy_has_dll;
	u32 phy_ren;
	u32 phy_otap_dly;
	u32 phy_itap_dly;
	u32 phy_sel_strb;
	u32 phy_sel_dly_rxclk;
	u32 phy_sel_dly_txclk;
	bool enable_hs_cap;
	bool enable_sdr50_cap;
	bool enable_ddr50_cap;
	bool enable_sdr104_cap;
	bool enable_hs400_cap;
	bool dis_support_64b;
};

static void thunderbay_sdhci_phy_reg_update(void __iomem *reg_addr, u32 mask,
					 u32 offset, u32 val)
{
	u32 reg_val = 0;

	if (val == THUNDERBAY_SDHCI_PHY_PROP_INVALID_VALUE)
		return;

	reg_val = readl(reg_addr);
	reg_val &= ~mask;
	if (val)
		reg_val |= val << offset;
	writel(reg_val, reg_addr);
}

static void thunderbay_sdhci_phy_update_delays(struct sdhci_host *host)
{
	struct thunderbay_sdhci_priv *priv = host->mmc->priv;
	struct thunderbay_sd_mmc_phy_regs *phy_base =
		(struct thunderbay_sd_mmc_phy_regs *)priv->phy_addr;

	/* Update SEL_DLY_RXCLK configuration */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_SEL_DLY_RXCLK_MASK,
				     PHY_CFG_0_SEL_DLY_RXCLK_OFFSET,
				     priv->phy_sel_dly_rxclk);

	/* Update SEL_DLY_TXCLK configuration */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_SEL_DLY_TXCLK_MASK,
				     PHY_CFG_0_SEL_DLY_TXCLK_OFFSET,
				     priv->phy_sel_dly_txclk);

	/* Update OTAP delay configuration */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_OTAP_DLY_X_MASK,
				     PHY_CFG_0_OTAP_DLY_X_OFFSET,
				     priv->phy_otap_dly);

	/*
	 * Update ITAP delay configuration: assert ITAP_CHG_WIN while
	 * changing itap_dly_sel, then de-assert.
	 */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_ITAP_CHG_WIN_MASK,
				     PHY_CFG_0_ITAP_CHG_WIN_OFFSET, 1);

	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_ITAP_DLY_X_MASK,
				     PHY_CFG_0_ITAP_DLY_X_OFFSET,
				     priv->phy_itap_dly);

	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_ITAP_CHG_WIN_MASK,
				     PHY_CFG_0_ITAP_CHG_WIN_OFFSET, 0);
}

static int thunderbay_sdhci_phy_init(struct sdhci_host *host)
{
	struct thunderbay_sdhci_priv *priv = host->mmc->priv;
	struct thunderbay_sd_mmc_phy_regs *phy_base =
		(struct thunderbay_sd_mmc_phy_regs *)priv->phy_addr;
	u32 reg_val = 0;
	u32 clk_mhz = priv->clk_in / 1000000;

	/* Clock frequency must not exceed mask. */
	if (clk_mhz & (u32)(~CTRL_CFG_0_BASE_CLK_FREQ_MASK))
		return -EINVAL;

	/* Update base clock frequency for controller. */
	reg_val = readl(&phy_base->ctrl_cfg_0);
	reg_val &= ~(CTRL_CFG_0_BASE_CLK_FREQ_MASK
		     << CTRL_CFG_0_BASE_CLK_FREQ_OFFSET);
	reg_val |= (clk_mhz << CTRL_CFG_0_BASE_CLK_FREQ_OFFSET);

	/*
	 * Configure the capabilities of the controller to match
	 * what we require.
	 */
	reg_val &= ~(CTRL_CFG_0_SPEED_CAPS_MASK);
	if (priv->enable_hs_cap)
		reg_val |= BIT(CTRL_CFG_0_SUPPORT_HS_OFFSET);
	writel(reg_val, &phy_base->ctrl_cfg_0);

	reg_val = readl(&phy_base->ctrl_cfg_1);
	reg_val &= ~(CTRL_CFG_1_SPEED_CAPS_MASK);

	if(priv->dis_support_64b)
	/* Clear 64-bit support */
		reg_val &= ~(BIT(CTRL_CFG_1_SUPPORT_64B_OFFSET));

	if (priv->enable_sdr50_cap)
		reg_val |= BIT(CTRL_CFG_1_SUPPORT_SDR50_OFFSET);
	if (priv->enable_ddr50_cap)
		reg_val |= BIT(CTRL_CFG_1_SUPPORT_DDR50_OFFSET);
	if (priv->enable_sdr104_cap)
		reg_val |= BIT(CTRL_CFG_1_SUPPORT_SDR104_OFFSET);
	if (priv->enable_hs400_cap)
		reg_val |= BIT(CTRL_CFG_1_SUPPORT_HS400_OFFSET);
	writel(reg_val, &phy_base->ctrl_cfg_1);

	/* Update delays */
	thunderbay_sdhci_phy_update_delays(host);

#if 0  /* TBU reviewed SK:*/
	/* Update pull up configuration for controller. */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_1, PHY_CFG_1_REN_X_MASK,
				     PHY_CFG_1_REN_X_OFFSET, priv->phy_ren);

	/* Update SEL_STRB */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_2,
				     PHY_CFG_2_SEL_STRB_MASK,
				     PHY_CFG_2_SEL_STRB_OFFSET,
				     priv->phy_sel_strb);
#endif

	/* Set power up bit */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_PWR_DOWN_MASK,
				     PHY_CFG_0_PWR_DOWN_OFFSET,
				     PHY_CFG_0_PWR_DOWN_EN);

	return 0;
}

static bool thunderbay_sdhci_phy_slot_type_removable(struct sdhci_host *host)
{
	struct thunderbay_sdhci_priv *priv = host->mmc->priv;
	struct thunderbay_sd_mmc_phy_regs *phy_base =
		(struct thunderbay_sd_mmc_phy_regs *)priv->phy_addr;
	u32 reg_val = 0;

	reg_val = readl(&phy_base->ctrl_cfg_1);
	reg_val >>= CTRL_CFG_1_SLOT_TYPE_OFFSET;
	reg_val &= CTRL_CFG_1_SLOT_TYPE_MASK;
	if (reg_val == CTRL_CFG_1_SLOT_TYPE_REMOVABLE)
		return true;

	return false;
}

static void thunderbay_sdhci_phy_set_delay(struct sdhci_host *host,
					u8 itap_dly_ena, u8 itap_dly_sel,
					u8 otap_dly_ena, u8 otap_dly_sel)
{
	struct thunderbay_sdhci_priv *priv = host->mmc->priv;

	/* Sanity check input from board function. */
	if (itap_dly_ena & ~(PHY_CFG_0_ITAP_DLY_ENA_MASK))
		return;
	if (itap_dly_sel & ~(PHY_CFG_0_ITAP_DLY_SEL_MASK))
		return;
	if (otap_dly_ena & ~(PHY_CFG_0_OTAP_DLY_ENA_MASK))
		return;
	if (otap_dly_sel & ~(PHY_CFG_0_OTAP_DLY_SEL_MASK))
		return;

	/*
	 * Generate the value from user input, offset at 0, shift
	 * will be done in the reg write function.
	 */
	priv->phy_itap_dly =
		itap_dly_sel | (itap_dly_ena << PHY_CFG_0_ITAP_DLY_SEL_BITS);
	priv->phy_otap_dly =
		otap_dly_sel | (otap_dly_ena << PHY_CFG_0_OTAP_DLY_SEL_BITS);

	thunderbay_sdhci_phy_update_delays(host);
}

static int thunderbay_sdhci_wait_for_card_detect(struct sdhci_host *host)
{
	u32 old_signal_en;
	u32 old_int_en;
	u32 int_status;
	unsigned long timeout_ms = THUNDERBAY_SDHCI_CD_TIMEOUT_MS;

	/* Save off old states */
	old_signal_en = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);
	old_int_en = sdhci_readl(host, SDHCI_INT_ENABLE);

	/* Enable card signals */
	sdhci_writel(host, SDHCI_INT_NORMAL_MASK, SDHCI_SIGNAL_ENABLE);
	sdhci_writel(host, SDHCI_INT_NORMAL_MASK, SDHCI_INT_ENABLE);

	/* Wait for card to appear */
	while (timeout_ms) {
		int_status = sdhci_readl(host, SDHCI_INT_STATUS);
		if (int_status & SDHCI_INT_CARD_INSERT) {
			/* Clear the flag */
			sdhci_writel(host, SDHCI_INT_CARD_INSERT,
				     SDHCI_INT_STATUS);
			break;
		}
		timeout_ms--;
		udelay(1000);
	}

	/* Restore old state */
	sdhci_writel(host, old_int_en, SDHCI_INT_ENABLE);
	sdhci_writel(host, old_signal_en, SDHCI_SIGNAL_ENABLE);

	if (timeout_ms == 0) {
		log_err("thunderbay_sdhci@%p: Timeout waiting for card detect\n",
			host->ioaddr);
		return -1;
	}

	return 0;
}

static int thunderbay_sdhci_force_card_insert(struct sdhci_host *host)
{
	u8 control;

	/* Force card detect */
	control = sdhci_readb(host, SDHCI_HOST_CONTROL);
	control |= (SDHCI_CTRL_CD_TEST_INS | SDHCI_CTRL_CD_TEST);
	sdhci_writeb(host, control, SDHCI_HOST_CONTROL);

	return 0;
}

#ifdef MMC_SUPPORTS_TUNING
/**
 * thunderbay_sdhci_send_tuning_command() - send a tuning-specific command
 * @host:		SDHCI host pointer
 * @opcode:		Command, which defines the tuning mode also
 *
 * We require a special version of send_cmd for tuning here, because the
 * controller behaviour differs from normal during the tuning sequence.
 *
 * Return: 0 or a negative error value.
 */
static int thunderbay_sdhci_send_tuning_command(struct sdhci_host *host, u8 opcode)
{
	struct mmc *mmc = host->mmc;
	struct mmc_cmd cmd;
	int size = 64;
	int blocks = 1;

	if (opcode == MMC_CMD_SEND_TUNING_BLOCK_HS200 && mmc->bus_width == 8)
		size = 128;

	cmd.cmdidx = opcode;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;

	/*
	 * Set up data size/direction for controller, the data will be
	 * transferred from card to host controller, even though we pass
	 * null pointer to mmc_send_cmd. We do this because we don't want
	 * the dm_mmc_send_cmd to do the normal data processing, as the
	 * flow is different for tuning.
	 */
	sdhci_writew(host, SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG, size),
		     SDHCI_BLOCK_SIZE);
	sdhci_writew(host, blocks, SDHCI_BLOCK_COUNT);
	sdhci_writew(host, SDHCI_TRNS_READ, SDHCI_TRANSFER_MODE);

	/*
	 * Send command - this function will handle the buffer read ready
	 * event for us.
	 */
	return dm_mmc_send_cmd(mmc->dev, &cmd, NULL);
}

static int thunderbay_sdhci_execute_tuning(struct mmc *mmc, u8 opcode)
{
	struct thunderbay_sdhci_priv *priv;
	struct sdhci_host *host;
	u16 host_ctrl2 = 0;
	int loops = 0;

	/* Get back the sdhci_host from struct mmc */
	priv = dev_get_priv(mmc->dev);
	host = &priv->host;

	/* Set 'execute tuning' */
	host_ctrl2 = sdhci_readw(host, SDHCI_HOST_CONTROL_2);
	host_ctrl2 |= SDHCI_CTRL2_EXECUTE_TUNING;
	sdhci_writew(host, host_ctrl2, SDHCI_HOST_CONTROL_2);

	/* Issue 'opcode' repeatedly until controller resets
	 * 'execute tuning' bit to 0.
	 * Controller will clear 'execute tuning' bit after 40 times
	 * repeating the sequence.
	 */
	for (loops = 0; loops < THUNDERBAY_SDHCI_TUNING_LOOPS; loops++) {
		/*
		 * Ignoring the return value of 'send_command'. There
		 * may be some errors returned during tuning sequence.
		 */
		thunderbay_sdhci_send_tuning_command(host, opcode);

		host_ctrl2 = sdhci_readw(host, SDHCI_HOST_CONTROL_2);
		if (!(host_ctrl2 & SDHCI_CTRL2_EXECUTE_TUNING))
			break;
	}

	/* Get latest state */
	host_ctrl2 = sdhci_readw(host, SDHCI_HOST_CONTROL_2);

	/* If 'execute tuning' still set, it means the sequence finished
	 * early, through a command error. Cancel tuning sequence.
	 */
	if (host_ctrl2 & SDHCI_CTRL2_EXECUTE_TUNING) {
		host_ctrl2 &= ~(SDHCI_CTRL2_EXECUTE_TUNING |
				SDHCI_CTRL2_SAMPLING_CLOCK_SELECT);
		sdhci_writew(host, host_ctrl2, SDHCI_HOST_CONTROL_2);
		debug("sdhci@%p: tuning failed - cancelled.\n", host->ioaddr);
	}

	/* If 'sampling clock select' set, then tuning succeeded. */
	if (host_ctrl2 & SDHCI_CTRL2_SAMPLING_CLOCK_SELECT) {
		debug("sdhci@%p: tuning succeeded\n", host->ioaddr);
		return 0;
	}

	debug("sdhci@%p: tuning failed - timed out\n", host->ioaddr);

	return -EIO;
}
#endif /* defined MMC_SUPPORTS_TUNING */

/* Boards may over-ride this and return sensible values for the board. */
int __weak board_thunderbay_sdhci_get_delay(struct mmc *mmc, u8 *itap_dly_ena,
					 u8 *itap_dly_sel, u8 *otap_dly_ena,
					 u8 *otap_dly_sel)
{
	/* Return -1 to indicate to the PHY driver not to update the delay */
	return -1;
}

static int thunderbay_sdhci_fix_card_detect(struct sdhci_host *host)
{
	struct thunderbay_sdhci_priv *priv = host->mmc->priv;
	u32 int_en;
	u8 control;
	int ret;

	/*
	 * Check if we are a removable device.
	 *
	 * If we are removable, we need to enable card detect.
	 *
	 * If we are removable AND have 'broken-cd', we require card detect
	 * forcing.
	 *
	 * We can only know if we are removable if we have the PHY to
	 * read.
	 *
	 * If we can't check if we are removable, assume nothing to do.
	 */
	if (!(priv->phy_addr && thunderbay_sdhci_phy_slot_type_removable(host)))
		return 0;

	/*
	 * In general, we need to enable the card detect events for the
	 * controller to be able to detect a card inserted, before it
	 * enables the power.
	 */
	if (!priv->broken_cd) {
		int_en = sdhci_readl(host, SDHCI_INT_ENABLE);

		int_en |= SDHCI_INT_CARD_INT | SDHCI_INT_CARD_INSERT |
			  SDHCI_INT_CARD_REMOVE;

		sdhci_writel(host, int_en, SDHCI_INT_ENABLE);
	} else {
		/*
		 * If the slot type is 'removable', i.e. SD card, and the
		 * card detect is broken/not connected, we can still force card
		 * detect here to allow us to communicate with the device.
		 */
		ret = thunderbay_sdhci_force_card_insert(host);
		if (ret)
			return ret;
	}

	/*
	 * We have to wait for card to appear (for both forced and normal
	 * operation) before enabling power.
	 *
	 * If the card is not present, we will have to wait here until
	 * the timeout completes.
	 */
	ret = thunderbay_sdhci_wait_for_card_detect(host);
	if (ret)
		return ret;

	/* Set power on */
	control = sdhci_readb(host, SDHCI_POWER_CONTROL);
	control |= SDHCI_POWER_ON;
	sdhci_writeb(host, control, SDHCI_POWER_CONTROL);

	return 0;
}

static void thunderbay_sdhci_set_control_reg(struct sdhci_host *host)
{
	u16 host_ctrl2 = 0;
	struct mmc *mmc = (struct mmc *)host->mmc;

	host_ctrl2 = sdhci_readw(host, SDHCI_HOST_CONTROL_2);
	host_ctrl2 &= ~THUNDERBAY_SDHCI_HOST_CTRL2_UHS_MASK;

	switch (mmc->selected_mode) {
	default:
		break;
	case MMC_HS_200:
	case UHS_SDR104:
		host_ctrl2 |= THUNDERBAY_SDHCI_HOST_CTRL2_UHS_SDR104;
		break;
	case MMC_DDR_52:
	case UHS_DDR50:
		host_ctrl2 |= THUNDERBAY_SDHCI_HOST_CTRL2_UHS_DDR50;
		break;
	case UHS_SDR50:
		host_ctrl2 |= THUNDERBAY_SDHCI_HOST_CTRL2_UHS_SDR50;
		break;
	case MMC_HS:
	case SD_HS:
		host_ctrl2 |= THUNDERBAY_SDHCI_HOST_CTRL2_UHS_SDR25;
		break;
	}

	sdhci_writew(host, host_ctrl2, SDHCI_HOST_CONTROL_2);
}

static int thunderbay_sdhci_set_ios_post(struct sdhci_host *host)
{
	struct thunderbay_sdhci_priv *priv = host->mmc->priv;
	struct thunderbay_sd_mmc_phy_regs *phy_base =
		(struct thunderbay_sd_mmc_phy_regs *)priv->phy_addr;
	struct mmc *mmc = (struct mmc *)host->mmc;
	u32 dll_freq_val = 0;
	u32 freq_mhz = 0;

	/* This PHY didn't have a DLL */
	if (!priv->phy_has_dll)
		return 0;

	/* Don't do reconfiguration when clock is being turned off. */
	if (mmc->clock == 0)
		return 0;

	/* Clear DLL_EN */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_DLL_EN_MASK,
				     PHY_CFG_0_DLL_EN_OFFSET, 0);

	/* No DLL required for < 50MHz */
	if(mmc->clock < (50*1000000))
		return 0;

	/* Get required DLL FREQ value */
	freq_mhz = mmc->clock / 1000000;

	if (freq_mhz <= 80)
		dll_freq_val = PHY_CFG_2_SEL_FREQ_80_50;
	else if (freq_mhz <= 110)
		dll_freq_val = PHY_CFG_2_SEL_FREQ_110_80;
	else if (freq_mhz <= 140)
		dll_freq_val = PHY_CFG_2_SEL_FREQ_140_110;
	else if (freq_mhz <= 170)
		dll_freq_val = PHY_CFG_2_SEL_FREQ_170_140;
	else if (freq_mhz <= 200)
		dll_freq_val = PHY_CFG_2_SEL_FREQ_200_170;
	else if (freq_mhz <= 225)
		dll_freq_val = PHY_CFG_2_SEL_FREQ_225_200;
	else if (freq_mhz <= 250)
		dll_freq_val = PHY_CFG_2_SEL_FREQ_250_225;
	else
		dll_freq_val = PHY_CFG_2_SEL_FREQ_275_250;

	/* Update value in PHY_CFG_2 */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_2,
				     PHY_CFG_2_SEL_FREQ_MASK,
				     PHY_CFG_2_SEL_FREQ_OFFSET, dll_freq_val);

	/* Set DLL_EN */
	thunderbay_sdhci_phy_reg_update(&phy_base->phy_cfg_0,
				     PHY_CFG_0_DLL_EN_MASK,
				     PHY_CFG_0_DLL_EN_OFFSET, 1);

	/* Wait for DLL */
	while (!(readl(&phy_base->phy_stat) & (PHY_STAT_DLL_RDY_MASK)))
		;

	return 0;
}

static void thunderbay_sdhci_set_delay(struct sdhci_host *host)
{
	struct thunderbay_sdhci_priv *priv = host->mmc->priv;
	struct mmc *mmc = (struct mmc *)host->mmc;
	u8 itap_dly_ena;
	u8 itap_dly_sel;
	u8 otap_dly_ena;
	u8 otap_dly_sel;
	int rc;

	/* If we don't have a PHY, nothing we can do here. */
	if (!priv->phy_addr)
		return;

	/*
	 * If the board returns non-zero, it means we don't want to update
	 * the delay values.
	 */
	rc = board_thunderbay_sdhci_get_delay(mmc, &itap_dly_ena, &itap_dly_sel,
					   &otap_dly_ena, &otap_dly_sel);
	if (rc)
		return;

	thunderbay_sdhci_phy_set_delay(host, itap_dly_ena, itap_dly_sel,
				    otap_dly_ena, otap_dly_sel);
}

static void thunderbay_sdhci_retrieve_phy_ofdata(struct thunderbay_sdhci_priv *priv,
					      int offset)
{
	priv->phy_ren =
	fdt_getprop_u32_default_node(gd->fdt_blob, offset, 0,
				     "intel,thunderbay-emmc-phy-ren",
				     THUNDERBAY_SDHCI_PHY_PROP_INVALID_VALUE);

	priv->phy_otap_dly =
	fdt_getprop_u32_default_node(gd->fdt_blob, offset, 0,
				     "intel,thunderbay-emmc-phy-otap-dly",
				     THUNDERBAY_SDHCI_PHY_PROP_INVALID_VALUE);

	priv->phy_itap_dly =
	fdt_getprop_u32_default_node(gd->fdt_blob, offset, 0,
				     "intel,thunderbay-emmc-phy-itap-dly",
				     THUNDERBAY_SDHCI_PHY_PROP_INVALID_VALUE);

	priv->phy_sel_strb =
	fdt_getprop_u32_default_node(gd->fdt_blob, offset, 0,
				     "intel,thunderbay-emmc-phy-sel-strb",
				     THUNDERBAY_SDHCI_PHY_PROP_INVALID_VALUE);

	priv->phy_sel_dly_rxclk =
	fdt_getprop_u32_default_node(gd->fdt_blob, offset, 0,
				     "intel,thunderbay-emmc-phy-sel-dly-rxclk",
				     THUNDERBAY_SDHCI_PHY_PROP_INVALID_VALUE);

	priv->phy_sel_dly_txclk =
	fdt_getprop_u32_default_node(gd->fdt_blob, offset, 0,
				     "intel,thunderbay-emmc-phy-sel-dly-txclk",
				     THUNDERBAY_SDHCI_PHY_PROP_INVALID_VALUE);

	priv->phy_has_dll = false;
	if (!fdt_node_check_compatible(gd->fdt_blob, offset,
				       "intel,thunderbay-emmc-phy"))
		priv->phy_has_dll = true;
}

static int thunderbay_sdhci_ofdata_to_platdata(struct udevice *dev)
{
	struct thunderbay_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	int offset = 0;
	int node = dev_of_offset(dev);
	int parent = dev_of_offset(dev->parent);

	host->name = strdup(dev->name);
	host->ioaddr = (void *)devfdt_get_addr(dev);
	if (!host->ioaddr) {
		debug("No register base address for this device.\n");
		return -ENXIO;
	}

	priv->f_max = dev_read_u32_default(dev, "max-frequency",
					   CONFIG_THUNDERBAY_SDHCI_MAX_FREQ);

	host->bus_width = dev_read_u32_default(dev, "bus-width", 4);

	/* Assume card detect not broken. */
	priv->broken_cd = dev_read_bool(dev, "broken-cd");

	/* Enable hardware high speed capability */
	priv->enable_hs_cap = dev_read_bool(dev, "cap-sd-highspeed") |
		dev_read_bool(dev, "cap-mmc-highspeed");
	/* Enable hardware SDR50 capability */
	priv->enable_sdr50_cap = dev_read_bool(dev, "sd-uhs-sdr50") |
		dev_read_bool(dev, "sd-uhs-sdr12") |
		dev_read_bool(dev, "sd-uhs-sdr25");
	/* Enable hardware DDR capability */
	priv->enable_ddr50_cap = dev_read_bool(dev, "sd-uhs-ddr50") |
		dev_read_bool(dev, "mmc-ddr-1_8v");
	/* Enable hardware SDR104/HS200 capability */
	priv->enable_sdr104_cap = dev_read_bool(dev, "sd-uhs-sdr104") |
		dev_read_bool(dev, "mmc-hs200-1_8v");
	/* Enable hardware HS400 capability */
	priv->enable_hs400_cap = dev_read_bool(dev, "mmc-hs400-1_8v");

	/* Disable 64-bit System Bus Support */
	priv->dis_support_64b = dev_read_bool(dev, "dis_support_64b");

	/*
	 * Look up phy for this device. If there is no phy, continue,
	 * but we may not be able to go to all modes.
	 */
	priv->phy_addr = NULL;
	offset = fdtdec_lookup_phandle(gd->fdt_blob, node, "phys");
	if (offset <= 0) {
		debug("No PHY for this device defined.\n");
		return 0;
	}

	priv->phy_addr =
	(void __iomem *)fdtdec_get_addr_size_auto_parent(gd->fdt_blob, parent,
							 offset, "reg", 0,
							 NULL, true);
	if (!priv->phy_addr) {
		debug("No register base address for this device's PHY.\n");
		return 0;
	}

	thunderbay_sdhci_retrieve_phy_ofdata(priv, offset);

	return 0;
}

static int thunderbay_sdhci_bind(struct udevice *dev)
{
	struct thunderbay_sdhci_plat *plat = dev_get_plat(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->config);
}

static struct sdhci_ops thunderbay_sdhci_ops = {
#ifdef MMC_SUPPORTS_TUNING
	.platform_execute_tuning = thunderbay_sdhci_execute_tuning,
#endif
	.set_delay = thunderbay_sdhci_set_delay,
	.set_ios_post = thunderbay_sdhci_set_ios_post,
	.set_control_reg = thunderbay_sdhci_set_control_reg
};

static int thunderbay_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct thunderbay_sdhci_plat *plat = dev_get_plat(dev);
	struct thunderbay_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	int ret;
	struct clk clk;

	/* Enable the controller. */
	ret = clk_get_by_index(dev, 1, &clk);
	if (ret < 0)
		return ret;

	ret = clk_enable(&clk);
	if (ret)
		return ret;

	/* Set rate and enable the xin clock. */
	ret = clk_get_by_index(dev, 0, &clk);
	if (ret < 0)
		return ret;

	priv->clk_in = clk_set_rate(&clk, priv->f_max);
	if (IS_ERR_VALUE(priv->clk_in))
		return priv->clk_in;

	ret = clk_enable(&clk);
	if (ret && ret != -ENOSYS)
		return ret;

	host->max_clk = priv->clk_in;
	host->mmc = &plat->mmc;
	host->mmc->priv = priv;
	host->ops = &thunderbay_sdhci_ops;

	if (priv->phy_addr) {
		ret = thunderbay_sdhci_phy_init(host);
		if (ret)
			return ret;
	}

	/*
	 * We cannot disable high speed mode through capabilities, it
	 * is assumed the controller supports it. We can disable via
	 * 'host->quirks', though.
	 */
	if (!priv->enable_hs_cap)
		host->quirks = SDHCI_QUIRK_BROKEN_HISPD_MODE;
	else
		host->quirks = 0;

	/*
	 * Note: if we enable > 32-bits of memory space in U-Boot, the
	 * sdhci driver will fail as SDMA mode on our controller only
	 * works with 32-bit addresses. In that case, set
	 * SDHCI_QUIRK_32BIT_DMA_ADDR in 'host->quirks' here, and define
	 * a memory address below 32-bit limit using
	 * CONFIG_FIXED_SDHCI_ALIGNED_BUFFER in the config.
	 */
#ifdef CONFIG_FIXED_SDHCI_ALIGNED_BUFFER
	host->quirks |= SDHCI_QUIRK_32BIT_DMA_ADDR;
#endif /* CONFIG_FIXED_SDHCI_ALIGNED_BUFFER */

	host->quirks |= SDHCI_QUIRK_BROKEN_R1B;

	/*
	 * Add 8-bit to the available bit widths, it won't be set
	 * otherwise.
	 */
	if (host->bus_width == 8)
		host->host_caps |= MMC_MODE_8BIT;

	/*
	 * Core sdhci code doesn't support HS 400, so add it
	 * to the caps here.
	 */
#if defined(CONFIG_MMC_HS400_SUPPORT)
	if (priv->enable_hs400_cap)
		host->host_caps |= MMC_CAP(MMC_HS_400);
#endif

	ret = sdhci_setup_cfg(&plat->config, host, priv->f_max,
			      CONFIG_THUNDERBAY_SDHCI_MIN_FREQ);
	if (ret)
		return ret;

	host->mmc->priv = &priv->host;
	host->mmc->dev = dev;
	upriv->mmc = host->mmc;

	ret = sdhci_probe(dev);
	if (ret)
		return ret;

	/* The 'sdhci_probe' function above will override the signal
	 * enables of the sdhci controller.
	 * This has the effect of masking the card detect signals that
	 * the controller is generating, and the power enable bit gets
	 * cleared immediately after being set.
	 * The controller will not function with the power enable bit
	 * cleared.
	 * We can't even do this in the 'get_cd' callback, because the
	 * signal enables get written *after* the host->ops->get_cd
	 * is called.
	 *
	 * So as a workaround for this, we need to handle two cases:
	 *
	 * (i) the normal case, we need to re-enable the card
	 *     detect signals when the card detect is functioning normally
	 * (ii) the broken card detect case, where we need to force
	 *      the card detection
	 */
	ret = thunderbay_sdhci_fix_card_detect(host);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id thunderbay_sdhci_ids[] = {
	{.compatible = "arasan,sdhci-5.1" },
	{}
};

U_BOOT_DRIVER(thunderbay_sdhci) = {
	.name = "thunderbay-sdhci",
	.id = UCLASS_MMC,
	.of_match = thunderbay_sdhci_ids,
	.of_to_plat = thunderbay_sdhci_ofdata_to_platdata,
	.ops = &sdhci_ops,
	.bind = thunderbay_sdhci_bind,
	.probe = thunderbay_sdhci_probe,
	.priv_auto = sizeof(struct thunderbay_sdhci_priv),
	.plat_auto = sizeof(struct thunderbay_sdhci_plat),
};
