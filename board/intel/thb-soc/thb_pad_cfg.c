// SPDX-License-Identifier: GPL-2.0                 v8.
/*
 * Intel Thunder Bay SoC pinctrl/GPIO driver
 *
 * Copyright (C) 2020 Intel Corporation
 */
#include <linux/io.h>
#include <asm/arch-thb/gpio.h>
#include "thb_pad_cfg.h"

struct pad_control {
	u32 index;  /* Pin number */
	u32 offset; /* Offset address */
	u32 value;  /* Desired value to be configured */
};

const struct pad_control evt1_pad[] ={
	{26, 0x068, 0x00170000},
	{27, 0x06C, 0x00170000},
	{28, 0x070, 0x00170000},
	{29, 0x074, 0x00170000},
	{30, 0x078, 0x00170000},
	{31, 0x07C, 0x00170000},
	{42, 0x0A8, 0x00170000},
	{43, 0x0AC, 0x00170000},
	{44, 0x0B0, 0x00170000},
	{45, 0x0B4, 0x00170000},
	{46, 0x0B8, 0x00170000},
	{47, 0x0BC, 0x00170000},
};

const struct pad_control evt2_pad[] = {
	{9, 0x024, 0x00000104},
	{14, 0x038, 0x00071504},
	{18, 0x048, 0x00000104},
	{21, 0x054, 0x00070104},
	{26, 0x068, 0x00000104},
	{35, 0x08C, 0x00070104},
};

void evt1_pad_config()
{
	u32 pin;
	u32 number_of_pins = sizeof(evt1_pad)/sizeof(evt1_pad[0]);
	for (pin = 0; pin < number_of_pins; pin++)
	{
		writel(evt1_pad[pin].value, GPIO_BASE_ADDR + evt1_pad[pin].offset);

	}
	readl(GPIO_BASE_ADDR);
}

void evt2_pad_config()
{
	u32 pin;
	u32 number_of_pins = sizeof(evt2_pad)/sizeof(evt2_pad[0]);
	for (pin = 0; pin < number_of_pins; pin++)
	{
		writel(evt2_pad[pin].value, GPIO_BASE_ADDR + evt2_pad[pin].offset);
	}
	readl(GPIO_BASE_ADDR);
}
