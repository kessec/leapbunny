/* lfp100.h --	support LFP100 chip.
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LFP100_H__
#define __LFP100_H__

#define LFP100_I2C_CHANNEL  0

#define LFP100_ADDR	0xCC

#define LFP100_CHIPID	0x00
#define LFP100_CONTROL	0x01
#define LFP100_STATUS1	0x02
#define LFP100_WLED	0x0A
#define LFP100_P_ENABLE	0x0E

/* CHIPID register */
#define CHIPID_CHIPID	(0xF << 4)
#define CHIPID_REV	(0xF << 0)

/* CONTROL register */
#define CONTROL_OFF	(0x1 << 4)
#define CONTROL_STANDBY	(0x1 << 3)
#define CONTROL_ACTIVE	(0x1 << 2)
#define CONTROL_MODE	(0x3 << 0)

#define CONTROL_MODE_ACTIVE   0x3
#define CONTROL_MODE_FAULT    0x2
#define CONTROL_MODE_STANDBY  0x1
#define CONTROL_MODE_OFF      0x0

/* STATUS1 register */
#define STATUS1_PB	(0x1 << 5)
#define STATUS1_SOURCE	(0x3 << 3)
#define STATUS1_SOURCE_AC	(0x3 << 3)
#define STATUS1_SOURCE_USB	(0x2 << 3)
#define STATUS1_SOURCE_BAT	(0x1 << 3)
#define STATUS1_SOURCE_UNDEF	(0x0 << 3)

#define STATUS1_AC	(0x1 << 2)
#define STATUS1_USB	(0x1 << 1)
#define STATUS1_BAT	(0x1 << 0)

/* WLED register */
/* set LFP100 to second brightest backlight level */
#define WLED_INITIAL	0x19

/* P_ENABLE register */
#define P_ENABLE_ALDO_EN	(0x1 << 6)
#define P_ENABLE_WLED_EN	(0x1 << 5)
#define P_ENABLE_LDO3_EN	(0x1 << 4)
#define P_ENABLE_LDO2_EN	(0x1 << 3)
#define P_ENABLE_LDO1_EN	(0x1 << 2)
#define P_ENABLE_DC2_EN		(0x1 << 1)
#define P_ENABLE_DC1_EN		(0x1 << 0)


enum lfp100_power_source {
	NOT_DEFINED = 0,
	BATTERY     = 1,
	USB         = 2,
	AC          = 3,
};

enum lfp100_power_source lfp100_get_power_source(void);

int lfp100_backlight_enable(void);
int lfp100_have_chip(void);
int lfp100_power_off(void);
int lfp100_get_power_button(void);
int lfp100_read(u8 reg, u8 *dest);
int lfp100_write(u8 reg, u8 val);

#endif /* __LFP100_H__ */
