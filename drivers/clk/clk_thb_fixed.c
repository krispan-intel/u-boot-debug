// SPDX-License-Identifier: GPL-2.0+
/*Copyright (c) 2020 Intel Corporation.
*/

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/clk-provider.h>

static ulong clk_thb_fixed_get_rate(struct clk *clk)
{
	return to_clk_fixed_rate(clk->dev)->fixed_rate;
}

static ulong clk_thb_fixed_set_rate(struct clk *clk, ulong rate)
{
	debug("%s:%ld\n",__func__,rate);
	/* Add Validity checks SK: */
	/*if(rate > eMMC(200MHz))
		return -EINVAL;*/
	return rate;
}


static int clk_thb_fixed_enable(struct clk *clk)
{
	/* Clocks are already enabled */
	debug("%s:%s\n",__func__,clk->dev->name);

	return 0;
}

const struct clk_ops clk_thb_fixed_ops = {
	.get_rate = clk_thb_fixed_get_rate,
	.set_rate = clk_thb_fixed_set_rate,
	.enable = clk_thb_fixed_enable,
};

static int clk_thb_fixed_ofdata_to_platdata(struct udevice *dev)
{
	struct clk *clk = &to_clk_fixed_rate(dev)->clk;
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	to_clk_fixed_rate(dev)->fixed_rate =
		dev_read_u32_default(dev, "clock-frequency", 0);
#endif
	/* Make fixed rate clock accessible from higher level struct clk */
	dev->uclass_priv_ = clk;
	clk->dev = dev;
	clk->enable_count = 0;

	return 0;
}

static const struct udevice_id clk_thb_fixed_match[] = {
	{
		.compatible = "thb-fixed-clock",
	},
	{ }
};

U_BOOT_DRIVER(clk_thb_fixed) = {
	.name = "clk_thb_fixed",
	.id = UCLASS_CLK,
	.of_match = clk_thb_fixed_match,
	.of_to_plat = clk_thb_fixed_ofdata_to_platdata,
	.plat_auto = sizeof(struct clk_fixed_rate),
	.ops = &clk_thb_fixed_ops,
};
