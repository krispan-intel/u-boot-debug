// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (C) 2020 Intel Corporation.
 */

#include <common.h>
#include <cpu_func.h>
#include <stdio.h>
#include <misc.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <asm/arch/thb_pcie.h>
#include <asm/arch/thb_pcie_boot.h>
#include <asm/io.h>
#include <dm/fdtaddr.h>

struct thunderbay_pcie_priv {
	void *bar2_base;
	unsigned long dbi_base;
	bool enabled;
};

#ifndef CONFIG_THUNDERBAY_PCIEBOOT_BUF_ADDR
#error "CONFIG_THUNDERBAY_PCIEBOOT_BUF_ADDR not defined"
#endif /* CONFIG_THUNDERBAY_PCIEBOOT_BUF_ADDR */
#ifndef CONFIG_THUNDERBAY_PCIEBOOT_BUF_SIZE
#error "CONFIG_THUNDERBAY_PCIEBOOT_BUF_SIZE not defined"
#endif /* CONFIG_THUNDERBAY_PCIEBOOT_BUF_SIZE */

static int thb_pcie_dma_read(unsigned long dma_base, u64 src,
			     u64 dst, size_t length)
{
	int rc = 0;
	u32 src_low = (uint32_t)src;
	u32 dest_low = (uint32_t)dst;
	u32 src_high = (uint32_t)(src >> 32);
	u32 dest_high = (uint32_t)(dst >> 32);
	struct pcie_dma_chan *dma_chan = (struct pcie_dma_chan *)(dma_base +
		DMA_READ_CHAN0_OFFSET);
	struct pcie_dma_reg *dma_reg = (struct pcie_dma_reg *)(dma_base);

	invalidate_dcache_range(dst, dst + length);

	writel(length, &dma_chan->dma_transfer_size);

	writel(src_low, &dma_chan->dma_sar_low);
	writel(src_high, &dma_chan->dma_sar_high);
	writel(dest_low, &dma_chan->dma_dar_low);
	writel(dest_high, &dma_chan->dma_dar_high);

	/* Start DMA transfer. */
	writel(PCIE_DMA_COMMS_CHANNEL,
	       &dma_reg->dma_read_doorbell);

	/* Wait for DMA transfer to complete. */
	while (!(in_32((uint32_t*)&dma_reg->dma_read_int_status) &
		(PCIE_DMA_ABORT_INTERRUPT_CH_MASK((PCIE_DMA_COMMS_CHANNEL)) |
		PCIE_DMA_DONE_INTERRUPT_CH_MASK((PCIE_DMA_COMMS_CHANNEL))))) {
	}

	if (in_32(&dma_reg->dma_read_err_status_low) ||
	    in_32(&dma_reg->dma_read_err_status_high)) {
		log_err("PCIe ERROR STATUS LOW: 0x%x HIGH: 0x%x\n",
			in_32(&dma_reg->dma_read_err_status_low),
			in_32(&dma_reg->dma_read_err_status_high));
		rc = -EIO;
	}

	/* Clear the done interrupt. */
	writel(PCIE_DMA_DONE_INTERRUPT_CH_MASK(PCIE_DMA_COMMS_CHANNEL),
	       &dma_reg->dma_read_int_clear);

	return rc;
}

static void thb_pcie_dma_disable(unsigned long dma_base)
{
	struct pcie_dma_reg *dma_reg = (struct pcie_dma_reg *)(dma_base);
	volatile uint32_t *dma_read_en_reg = (uint32_t *)&dma_reg->dma_read_engine_en;

	/* Disable the DMA read engine. */
	writel(0, &dma_reg->dma_read_engine_en);

	/* Wait until the DMA read engine is disabled. */
	do{
	}while(readl(dma_read_en_reg) & PCIE_DMA_READ_ENGINE_EN_MASK);
}

static void thb_pcie_dma_init(unsigned long dma_base)
{
	struct pcie_dma_chan *dma_chan = (struct pcie_dma_chan *)(dma_base +
		DMA_READ_CHAN0_OFFSET);
	struct pcie_dma_reg *dma_reg = (struct pcie_dma_reg *)(dma_base);

	/* Disable the DMA read engine. */
	thb_pcie_dma_disable(dma_base);

	/* Enable the DMA read engine. */
	writel(PCIE_DMA_READ_ENGINE_EN_MASK, &dma_reg->dma_read_engine_en);

	/* Mask all interrupts. */
	writel(PCIE_DMA_ALL_INTERRUPT_MASK, &dma_reg->dma_read_int_mask);

	/* Clear all interrupts. */
	writel(PCIE_DMA_ALL_INTERRUPT_MASK, &dma_reg->dma_read_int_clear);

	/* On DMA read channel 0.
	 * Assumptions:
	 *    No address translation.
	 *    No traffic class.
	 *    No relaxed ordering.
	 *    Allow cache snooping (No need for cache flush's).
	 *    Function number is 0.
	 */
	/* Enable local interrupt status (LIE). */
	writel(PCIE_DMA_CH_CONTROL1_LIE_MASK, &dma_chan->dma_ch_control1);
}

static void thb_pcie_iatu_bar_map(struct thunderbay_pcie_priv *priv,
				  u64 buf, u8 bar_no)
{
	unsigned long iatu_base = 0;
	struct pcie_iatu_inbound *iatu_region;
	struct pcie_type0_header *pcie_header =	(struct pcie_type0_header *)
		priv->dbi_base;
	uint32_t bar2_reg, bar3_reg;

	iatu_base = priv->dbi_base + IATU_DBI_OFFSET +
		IATU_INBOUND0_OFFSET;

	iatu_region = (struct pcie_iatu_inbound *)iatu_base;

      /* No configuration required for cfg1. */
        writel(0, &iatu_region->iatu_region_ctrl_1);
        /* Set target address for address mapping. */
        writel((u32)((buf) & (0xFFFFFFFF)),
               &iatu_region->iatu_lwr_target_addr);
        writel((u32)(buf >> 32),
               &iatu_region->iatu_upper_target_addr);

        /* Set the configuration 2 register. */
        writel(((1 << PCIE_IATU_CTRL_2_MATCH_MODE_SHIFT)
                | (bar_no << PCIE_IATU_CTRL_2_BAR_NUM_SHIFT)
                | (PCIE_IATU_CTRL_2_REGION_EN_MASK)),
                &iatu_region->iatu_region_ctrl_2);

}

/* Flush assumed of the PCIe comms register if updated before this
 * function. PCIe comms registers invalidated during polling of
 * interrupt registers.
 */
int thb_pcie_send_msi(struct thunderbay_pcie_priv *const priv)
{
	uint32_t msi_data;
	uint32_t msi_lwr_target_addr, msi_upr_target_addr;
	struct pcie_type0_header *pcie_header =	(struct pcie_type0_header *)
		priv->dbi_base;
	struct pcie_msi_capability  *msi_cap = (struct pcie_msi_capability *)
		(priv->dbi_base + PCIE_DBI_MSI_CAP_OFFSET);
	struct pcie_iatu_outbound *iatu_region =
			(struct pcie_iatu_outbound *)(priv->dbi_base + IATU_DBI_OFFSET + IATU_OUTBOUND0_OFFSET);

	debug("%s\n",__func__);

	/* Check MSI enable is set. */
	if (!(readw(&msi_cap->message_control) &
	    PCIE_CFG_MSI_CAP_MSI_EN_MASK))
		return -EIO;

	/* Check bus master enable set. */
	if (!(readw(&pcie_header->command) &
	    PCIE_HDR_COMMAND_BUS_MASTER_ENABLE_MASK))
		return -EIO;

        /* Clear bit 0-4 and 20-22 of IATU_REGION_CTRL_1_OFF_OUTBOUND_0 i.e func0*/
	clrbits_32(&iatu_region->iatu_region_ctrl_1, 0x700000);
	clrbits_32(&iatu_region->iatu_region_ctrl_1, 0x1F);

	/* Set bit 31 and 23 of IATU_REGION_CTRL_2_OFF_OUTBOUND_0 */
	setbits_32(&iatu_region->iatu_region_ctrl_2, BIT(PCIE_IATU_CTRL_2_REGION_EN_SHIFT)|BIT(23));

	writel((uint32_t)PCIE_CTRL_0_X2_USP_AXI_SLAVE_BASE,&iatu_region->iatu_lwr_base_addr);
	writel((uint32_t)(PCIE_CTRL_0_X2_USP_AXI_SLAVE_BASE >> 32), &iatu_region->iatu_upper_base_addr);

	msi_lwr_target_addr = readl(&msi_cap->message_address_lower);
	msi_upr_target_addr = readl(&msi_cap->message_address_upper);
	msi_data = readl(&msi_cap->message_data);

	writel(msi_lwr_target_addr, &iatu_region->iatu_lwr_target_addr);
	writel(msi_upr_target_addr, &iatu_region->iatu_upper_target_addr);

	writel(msi_data, PCIE_CTRL_0_X2_USP_AXI_SLAVE_BASE);

	return 0;
}

static int thb_do_iatu_setup(struct thunderbay_pcie_priv *const priv,
			     const unsigned long iatu_base,
			     struct pcie_iatu_setup_cfg *iatu_cfg)
{
	phys_addr_t bar2_phys = 0;
	int rc = 0;
	const char *vpu_magic = iatu_cfg->magic;
	const u32 bar2_size = iatu_cfg->comms_bar_size;
	const u64 dev_id = iatu_cfg->dev_id;
	const char *recov_str = VPURECOV_MAGIC_STRING;
	uint32_t int_mask = 0;
	int wait_host = 0;

	if (bar2_size == 0 || bar2_size > BAR2_MAX_SIZE)
		return -EINVAL;

	if (bar2_size <= CONFIG_THUNDERBAY_PCIEBOOT_BUF_SIZE) {
		if (CONFIG_THUNDERBAY_PCIEBOOT_BUF_ADDR & (bar2_size - 1))
			return -ENOMEM;
		priv->bar2_base = (void *)CONFIG_THUNDERBAY_PCIEBOOT_BUF_ADDR;
	} else {
		return -ENOMEM;
	}

#ifndef CONFIG_THUNDERBAY_PCIEBOOT_BUF_ADDR
	if (!priv->bar2_base) {
		priv->bar2_base = memalign(bar2_size,
					   VPUUBOOT_COMMS_BUFFER_SIZE);
		if (!priv->bar2_base)
			return -ENOMEM;
	}
#endif /* CONFIG_THUNDERBAY_PCIEBOOT_BUF_ADDR */

	wait_host = !memcmp((void *)vpu_magic, (void *)recov_str,
		VPUUBOOT_MAGIC_SIZE);

	if (!wait_host) {
		/* If the magic word indicates the VPU PCIe boot sequence has
		 * already occurred, set up the interrupt mask and enable
		 * communication registers for further communications.
		 */
		int_mask = VPUUBOOT_READY_INTERRUPT;
	}

	memset(priv->bar2_base, 0, VPUUBOOT_COMMS_BUFFER_SIZE);
	/* Write the device ID into the Communication BAR. */
	memcpy(priv->bar2_base + VPUUBOOT_DEV_ID_OFFSET, &dev_id,
	       sizeof(dev_id));
	/* Indicate to host THB PCIe EP is in U-Boot. */
	memcpy(priv->bar2_base, vpu_magic, VPUUBOOT_MAGIC_SIZE);
	/* Set the magic date. */
	writel(VPUUBOOT_MAGIC_DATE,
	       (priv->bar2_base + VPUUBOOT_MAGIC_SIZE));
	/* Assume READY interrupt is enabled. */
	writel(int_mask,
	       (priv->bar2_base + VPUUBOOT_INT_EN_OFFSET));
	/* Assume READY interrupt is unmasked. */
	writel((u32)(~int_mask),
	       (priv->bar2_base + VPUUBOOT_INT_MSK_OFFSET));

	bar2_phys = virt_to_phys(priv->bar2_base);

	if (!bar2_phys)
		return -ENOMEM;

	/* Set iATU to the allocated buffer and enable. */
	thb_pcie_iatu_bar_map(priv, (u64)bar2_phys, 2);

	/* Indicate ready and send MSI. */
	writel(VPUUBOOT_READY_INTERRUPT,
	       priv->bar2_base + VPUUBOOT_INT_IDENT_OFFSET);

	flush_dcache_range((unsigned long)priv->bar2_base,
			   (unsigned long)(priv->bar2_base +
			   VPUUBOOT_COMMS_BUFFER_SIZE));

	rc = thb_pcie_send_msi(priv);

	return rc;
}

static void thb_pcie_iatu_disable(const unsigned long iatu_base)
{
	struct pcie_iatu_inbound *iatu_region =
			(struct pcie_iatu_inbound *)iatu_base;
	int i = 0;

	for (i = 0; i < PCIE_IATU_NUM_REGIONS; i++) {
		/* Disable the outbound iATU 0. */
		writel(0, &iatu_region->iatu_region_ctrl_2);

		/* Index into next iATU region. */
		iatu_region = (struct pcie_iatu_inbound *)
				(iatu_base + IATU_INBOUND_REG_DIFF);
	}
}

int thb_pcie_ep_read(struct udevice *dev, int offset, void *buf, int size)
{
	/* Implemented on THB to read the physical memory of BAR2. */
	struct thunderbay_pcie_priv *priv = dev_get_priv(dev);

	if (!priv)
		return -ENODEV;

	if (!priv->enabled || !priv->bar2_base)
		return -EPERM;

	if (!buf || !size)
		return -EINVAL;

	if (offset < 0 || ((offset + size) > VPUUBOOT_COMMS_BUFFER_SIZE))
		return -EINVAL;

	invalidate_dcache_range((unsigned long)priv->bar2_base + offset,
				(unsigned long)priv->bar2_base + offset + size);
	memcpy(buf, priv->bar2_base + offset, size);

	return 0;
}

int thb_pcie_ep_write(struct udevice *dev, int offset, const void *buf,
		      int size)
{
	struct thunderbay_pcie_priv *priv = dev_get_priv(dev);
	if (!priv)
		return -ENODEV;

	if (!priv->enabled || !priv->bar2_base)
		return -EPERM;

	if (!buf || !size)
		return -EINVAL;

	if (offset < 0 || ((offset + size) > VPUUBOOT_COMMS_BUFFER_SIZE))
		return -EINVAL;

	/* Copy buffer to bar2 base + offset. */
	memcpy(priv->bar2_base + offset, buf, size);
	flush_dcache_range((unsigned long)priv->bar2_base + offset,
			   (unsigned long)(priv->bar2_base + offset + size));

	return 0;
}

int thb_pcie_ep_call(struct udevice *dev, int msgid, void *tx_msg,
		     int tx_size, void *rx_msg, int rx_size)
{
	struct thunderbay_pcie_priv *priv = dev_get_priv(dev);
	unsigned long dma_base;
	u64 src = 0;

	if (!priv)
		return -ENODEV;

	if (!priv->enabled)
		return -EPERM;

	if (!tx_msg || !rx_msg || !rx_size || !tx_size)
		return -EINVAL;

	switch (msgid) {
	/* ID ‘DMAR’: Implemented on THB to perform blocking PCIe DMA
	 * operations from the host physical memory.
	 */
	case PCIE_BOOT_DMA_READ_ID:
		if (tx_size > sizeof(u64))
			return -EINVAL;

		dma_base = priv->dbi_base + DMA_DBI_OFFSET;
		/* Retrieve the host source address. */
		memcpy((void *)&src, tx_msg, tx_size);

		return thb_pcie_dma_read(dma_base, src, (u64)rx_msg, rx_size);
	default:
		return -EINVAL;
	}

	return 0;
}

int thb_pcie_ep_ioctl(struct udevice *dev, unsigned long request, void *buf)
{
	struct thunderbay_pcie_priv *priv = dev_get_priv(dev);
	unsigned long iatu_base = 0;
	int rc = 0;

	if (!priv)
		return -ENODEV;

	iatu_base = priv->dbi_base + IATU_DBI_OFFSET +
		IATU_INBOUND0_OFFSET;

	switch (request) {
	case PCIE_BOOT_IATU_SETUP:
	{
		/* BAR2 size is located in the first 32b of the buffer. */
		rc = thb_do_iatu_setup(priv, iatu_base,
				       (struct pcie_iatu_setup_cfg *)buf);

		if (rc)
			return rc;

		break;
	}
	case PCIE_BOOT_BAR2_REMAP:
	{
		phys_addr_t bar2_phys = 0;
		u64 bar2_base = 0;

		if (priv->bar2_base) {
			free(priv->bar2_base);
			priv->bar2_base = 0;
		}

		memcpy((void *)&bar2_base, buf, sizeof(bar2_base));

		if (bar2_base == 0)
			return -EINVAL;

		bar2_phys = virt_to_phys((void *)bar2_base);

		/* Set iATU to the allocated buffer and enable. */
		thb_pcie_iatu_bar_map(priv, (u64)bar2_phys, 2);
		break;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

int thb_pcie_ep_set_enabled(struct udevice *dev, bool val)
{
	struct thunderbay_pcie_priv *priv = dev_get_priv(dev);
	unsigned long dma_base = 0;
	unsigned long iatu_base = 0;
	int rc = 0;

	if (!priv)
		return -ENODEV;

	if (priv->enabled)
		rc = 1;

	dma_base = priv->dbi_base + DMA_DBI_OFFSET;

	/* Enable: Enable DMA engine. */
	if (val) {
		thb_pcie_dma_init(dma_base);
		/* Setup iATU for BAR2 to configured base address done
		 * in ioctl
		 */
		struct pcie_iatu_outbound *iatu_region =
                        (struct pcie_iatu_outbound *)(priv->dbi_base + IATU_DBI_OFFSET + IATU_OUTBOUND0_OFFSET);
		/* Select function 0, write 0x0 to bit 20-22*/
		clrbits_32(&iatu_region->iatu_region_ctrl_1, 0x700000);
		/* Disable Transalation */
		clrbits_32(&iatu_region->iatu_region_ctrl_1, BIT(PCIE_IATU_CTRL_2_REGION_EN_SHIFT));

		priv->enabled = true;
	} else {
		iatu_base = priv->dbi_base + IATU_DBI_OFFSET +
			IATU_INBOUND0_OFFSET;
		/* Disable: Setup iATU to respond with UR for all accesses.
		 * Disable DMA engine.
		 */
		thb_pcie_dma_disable(dma_base);
		thb_pcie_iatu_disable(iatu_base);
		priv->enabled = false;
	}

	return rc;
}

int thb_pcie_ep_get_platdata(struct udevice *dev)
{
	struct thunderbay_pcie_priv *priv = dev_get_priv(dev);

	if (!priv)
		return -ENODEV;

	priv->dbi_base = devfdt_get_addr_name(dev, "dbi");

	if (priv->dbi_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	return 0;
}

int thb_pcie_ep_probe(struct udevice *dev)
{
	/* Retrieve private data buffer. */
	struct thunderbay_pcie_priv *priv = dev_get_priv(dev);

	if (!priv)
		return -ENODEV;

	memset((void *)priv, sizeof(struct thunderbay_pcie_priv), 0);

	return 0;
}

static const struct misc_ops thb_pcie_ep_ops = {
	.read = thb_pcie_ep_read,
	.write = thb_pcie_ep_write,
	.ioctl = thb_pcie_ep_ioctl,
	.call = thb_pcie_ep_call,
	.set_enabled = thb_pcie_ep_set_enabled,
};

static const struct udevice_id thb_pcie_ep_ids[] = {
	{ .compatible = "intel,thunderbay-pcie-ep" },
	{ }
};

U_BOOT_DRIVER(thb_pcie_ep) = {
	.name           = "thunderbay_pcie_boot_ep",
	.id             = UCLASS_MISC,
	.ops		= &thb_pcie_ep_ops,
	.of_match       = thb_pcie_ep_ids,
	.probe          = thb_pcie_ep_probe,
	.of_to_plat     = thb_pcie_ep_get_platdata,
	.priv_auto      = sizeof(struct thunderbay_pcie_priv),
};
