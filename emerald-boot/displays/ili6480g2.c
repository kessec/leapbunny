/* ili6480g2.c - Detect and initialize the ILI6480G2 LCD driver.
 *
 * This chip uses a three-wire bus with a bidirectional "data" pin so we
 * bit-bang it using GPIO pins.
 *
 * Copyright 2010-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdint.h>
#include <common.h>
#include <gpio.h>
#include <board.h>
#include <string.h>
#include <global.h>
#include <setup.h>
#include <debug.h>

#define TW_PORT         GPIO_PORT_B
#define TW_SCK_PIN      13
#define TW_SPD_PIN      15
#define TW_SPE_PIN      12
#define	DELAY		10

#define cs(v)		gpio_set_val(TW_PORT, TW_SPE_PIN, v)
#define clk(v)		gpio_set_val(TW_PORT, TW_SCK_PIN, v);

/* write out a bit: the device reads bits on a rising clock edge so we let the
 * clock go low, set the data bit, and then drive the clock high and wait for
 * the device to clock in the data bit. */
static void tx(unsigned int value)
{
	volatile int i;

	clk(0);
	gpio_set_val(TW_PORT, TW_SPD_PIN, !!value);
	for(i = 0; i < DELAY/2; i++);
	clk(1);
	for(i = 0; i < DELAY; i++);
}

/* read in a bit: the device expects us to read on a risking clock edge so we
 * let the clock go low, wait for the device to set the data bit, drive the
 * clock high, and then read the bit. */
static uint8_t rx(void)
{
	uint8_t val;
	volatile int i;

	clk(0);
	for(i = 0; i < DELAY/2; i++);
	val = gpio_get_val(TW_PORT, TW_SPD_PIN);
	clk(1);
	for(i = 0; i < DELAY; i++);

	return val;
}

/* the protocol is [6-bit address][1][1][8-bit data] */
static uint8_t ili6480g2_read_reg(uint8_t reg)
{
	volatile int i;
	uint8_t val = 0;

	gpio_set_out_en(TW_PORT, TW_SPD_PIN, 1);
	clk(0);
	cs(0);
	for(i = 0; i < DELAY*2; i++);

	/* send the address */
	for (i = 0; i < 6; i++)
		tx(reg & (1<<(5-i)));

	/* send the 'read' bit */
	tx(1);
	/* send the 'Hi-Z' bit */
	tx(1);

	/* receive the data */
	gpio_set_out_en(TW_PORT, TW_SPD_PIN, 0);
	for (i = 0; i < 8; i++)
		val |= (rx()<<(7-i));

	cs(1);

	return val;
}

/* the protocol is [6-bit address][0][1][8-bit data] */
static uint8_t ili6480g2_write_reg(uint8_t reg, uint8_t val)
{
	volatile int i;

	gpio_set_out_en(TW_PORT, TW_SPD_PIN, 1);
	clk(0);
	cs(0);
	for(i = 0; i < DELAY*2; i++);

	/* send the address */
	for (i = 0; i < 6; i++)
		tx(reg & (1<<(5-i)));

	/* send the 'write' bit */
	tx(0);
	/* send the 'Hi-Z' bit */
	tx(1);

	/* send the data */
	for (i = 0; i < 8; i++)
		tx(val & (1<<(7-i)));

	cs(1);

	return val;
}

/* This chip doesn't have a "chip ID" register or anything similar, however we
 * know some initial values so let's assume that we've found the chip if we
 * read expected values for a few registers. */
int ili6480g2_detect(void)
{
	gpio_configure_pin(TW_PORT, TW_SCK_PIN, GPIO_GPIOFN, 1, 0, 0);
	gpio_configure_pin(TW_PORT, TW_SPD_PIN, GPIO_GPIOFN, 1, 0, 0);
	gpio_configure_pin(TW_PORT, TW_SPE_PIN, GPIO_GPIOFN, 1, 0, 1);

	return (ili6480g2_read_reg(0) == 0x00 &&
		(ili6480g2_read_reg(1) & 0x1C) == 0x1C &&
	   	ili6480g2_read_reg(2) == 0x26 &&
	   	ili6480g2_read_reg(3) == 0x7F);
}

void ili6480g2_init(void)
{
	global_var * gptr = get_global();

	switch (gptr->board_id) {
	case LF1000_BOARD_MADRID:
	case LF1000_BOARD_MADRID_LFP100:
	case LF1000_BOARD_MADRID_POP:
		/* Reconfigure LCD to flip upside-down and rightside-left. */
		ili6480g2_write_reg(1, 0x1C);
		break;
	default:
		ili6480g2_write_reg(1, 0x1F);
		break;
	}
}
