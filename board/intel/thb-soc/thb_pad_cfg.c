// SPDX-License-Identifier: GPL-2.0                 v8.
/*
 * Intel Thunder Bay SoC pinctrl/GPIO driver
 *
 * Copyright (C) 2020 Intel Corporation
 */
#include <linux/io.h>
#include <asm/arch-thb/gpio.h>
#include <asm/arch/boot/platform_private.h>
#include "thb_pad_cfg.h"
#include "evt1_pcl_header_config_tbh.h"
#include "evt2_pcl_header_config_tbh.h"

void pcl_pad_write_config(const struct pcl_pad_control *config_pad, u32 pad_size)
{
/* Added a common function to write all the configurations */
	u32 pin;

	for (pin = 0; pin < pad_size; pin++)
		writel(config_pad[pin].value, GPIO_BASE_ADDR + (config_pad[pin].index) * 4);
	readl(GPIO_BASE_ADDR);
}

void pcl_pad_config(u8 board_id, u8 thb_full)
{
	u32 number_of_pins;

	if (board_id == BOARD_TYPE_HDDLF1 && thb_full) {
		number_of_pins = ARRAY_SIZE(evt1_full_pad);
		pr_info("Applying configuration for EVT1 Full\n");
		pcl_pad_write_config(evt1_full_pad, number_of_pins);
	} else if (board_id == BOARD_TYPE_HDDLF1 && !thb_full) {
		number_of_pins = ARRAY_SIZE(evt1_prime_pad);
		pr_info("Applying configuration for EVT1 Prime\n");
		pcl_pad_write_config(evt1_prime_pad, number_of_pins);
	} else if (board_id == BOARD_TYPE_HDDLF2 && thb_full) {
		number_of_pins = ARRAY_SIZE(evt2_full_pad);
		pr_info("Applying configuration for EVT2 Full\n");
		pcl_pad_write_config(evt2_full_pad, number_of_pins);
	} else if (board_id == BOARD_TYPE_HDDLF2 && !thb_full) {
		number_of_pins = ARRAY_SIZE(evt2_prime_pad);
		pr_info("Applying configuration for EVT2 Prime\n");
		pcl_pad_write_config(evt2_prime_pad, number_of_pins);
	}
}
