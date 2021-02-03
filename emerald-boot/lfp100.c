/* lfp100.c --  support LFP100 chip.  If present chip controls power,
 *		backlight, and audio.
 *
 * Copyright 2010-2011 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <lfp100.h>
#include <i2c.h>
#include <debug.h>

int lfp100_read(u8 reg, u8 *dest)
{
	int ret;
	u8 buf[2];

	db_puts("lfp100_read  reg=");db_byte(reg);db_putchar('\n');
	buf[0] = reg;
	ret = i2c_write(LFP100_I2C_CHANNEL, LFP100_ADDR, buf, 1);
	if (ret) {
		db_puts("A: lfp100_read ret(1)\n");
		return 1;
	}

	ret = i2c_read(LFP100_I2C_CHANNEL, LFP100_ADDR, buf, 2);
	if (ret) {
		db_puts("B: lfp100_read ret(1)\n");
		return 1;
	}

	db_puts("lfp100_read buf[1]=");
	db_byte(buf[1]);db_putchar('\n');
	*dest = buf[1];
	return 0;
}

int lfp100_write(u8 reg, u8 val)
{
	int ret;
	u8 buf[2];

	db_puts("lfp100_write  reg=");db_byte(reg);
	db_puts(" val=");db_byte(val);db_putchar('\n');
	buf[0] = reg;
	buf[1] = val;
	ret = i2c_write(LFP100_I2C_CHANNEL, LFP100_ADDR, buf, 2);
	db_puts("lfp100_write  ret=");
	db_int(ret);db_putchar('\n');
	return ret;
}

int lfp100_have_chip(void)
{
	int ret;
	u8 val;

	ret = lfp100_read(LFP100_CHIPID, &val);
	db_puts("ret="); db_int(ret); db_putchar('\n');
	db_puts("val="); db_byte(val); db_putchar('\n');
	if (ret) {
		serio_puts("NO LFP100\n");
		return(0);
	} else {
		serio_puts("Found LFP100 r"); serio_byte(val & 0xF0);
		serio_putchar('\n');
		return(1);
	}
}

int lfp100_power_off(void)
{
	int ret;
	u8 val;

	ret = lfp100_read(LFP100_CONTROL, &val);
	db_puts("lfp100_power_off ");
	db_puts("ret="); db_int(ret); db_putchar('\n');
	db_puts("val="); db_byte(val); db_putchar('\n');
	if (ret) {
		serio_puts("LFP100 read error\n");
		return(0);
	}
	val |= CONTROL_OFF;
	ret = lfp100_write(LFP100_CONTROL, val);
	return(ret);
}

enum lfp100_power_source lfp100_get_power_source(void)
{
	enum lfp100_power_source ret;
	u8 val;

	ret = lfp100_read(LFP100_STATUS1, &val);
        db_puts("ret="); db_int(ret); db_putchar('\n');
        db_puts("val="); db_byte(val); db_putchar('\n');
	if (ret) {
		serio_puts("lfp100_power_source ret=");serio_int(ret);
		serio_putchar('\n');
		return NOT_DEFINED;
	}

	db_puts("lfp100_get_power_source ret=");
	switch(val & STATUS1_SOURCE) {
		case STATUS1_SOURCE_BAT:
			ret = BATTERY;
			db_puts("BATTERY");
			break;
		case STATUS1_SOURCE_USB:
			ret = USB;
			db_puts("USB");
			break;
		case STATUS1_SOURCE_AC:
			ret = AC;
			db_puts("AC");
			break;
		case STATUS1_SOURCE_UNDEF:
		default:
			ret = NOT_DEFINED;
			db_puts("NOT_DEFINED");
			break;
	}
	db_putchar('\n');
	return ret;
}

int lfp100_get_power_button(void)
{
	int ret;
	u8 val;

	ret = lfp100_read(LFP100_STATUS1, &val);
        db_puts("ret="); db_int(ret); db_putchar('\n');
        db_puts("val="); db_byte(val); db_putchar('\n');
	if (ret) {
		serio_puts("lfp100_get_pb ret=");serio_int(ret);
		serio_putchar('\n');
		return NOT_DEFINED;
	}

	return (val & STATUS1_PB ? 1 : 0);
}

int lfp100_backlight_enable(void)
{
	int ret;
	u8 val;

	ret = lfp100_write(LFP100_WLED, WLED_INITIAL);
	if (ret) {
		db_puts("error: can't write WLED_INITIAL\n");
		return ret;
	}

	ret = lfp100_read(LFP100_P_ENABLE, &val);

	if (ret) {
		db_puts("error: can't read P_ENABLE\n");
		return ret;
	} else {
		ret = lfp100_write(LFP100_P_ENABLE, val | P_ENABLE_WLED_EN);
	}
	return ret;
}
