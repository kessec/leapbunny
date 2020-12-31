/* nt39016.c - Detect and initialize the NovaTek LCD driver.
 *
 * The NT39016 uses a three-wire (half-duplex SPI-style) bus which the LF1000
 * does not support.  We therefore emulate this bus by bit-banging the relevant
 * pins.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * Copyright 2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdint.h>
#include <mach/common.h>
#include <mach/gpio.h>
#include "include/autoconf.h"
#include "include/mach-types.h" /* for machine info */
#include "include/board.h"
#include "include/string.h"
#include "include/setup.h"
#include "include/debug.h"
#include "include/gpio.h"

#define TW_PORT		GPIO_PORT_B
#define TW_SCK_PIN	13
#define TW_SPD_PIN	14
#define TW_SPE_PIN	12
#define TW_TEST_PIN	15

#define DELAY		10

/* Try to detect the presense of an NT39016 chip and return 1 if the chip was
 * found or 0 if it was not found.
 *
 * We detect the chip by assuming that its cable shorts out our SPI0 TX and RX
 * pins (MOSI and MISO, espectively).  This means that we can set one pin as an
 * output and the other as an input and detect a loopback pattern. */
int nt39016_detect(void)
{
	int rx;

	gpio_configure_pin(TW_PORT, TW_SPD_PIN, GPIO_GPIOFN, 1, 0, 0);
	gpio_configure_pin(TW_PORT, TW_TEST_PIN, GPIO_GPIOFN, 0, 0, 0);

	rx = gpio_get_val(TW_PORT, TW_TEST_PIN);
	gpio_set_val(TW_PORT, TW_SPD_PIN, !rx);

	if (gpio_get_val(TW_PORT, TW_TEST_PIN) == !rx)
		return 1;

	return 0;
}

/* write out a bit: the device reads bits on a rising clock edge so we let the
 * clock go low, set the data bit, and then drive the clock high and wait for
 * the device to clock in the data bit. */
static void tx(unsigned int value)
{
	volatile int i;

	gpio_set_val(TW_PORT, TW_SCK_PIN, 0);
	gpio_set_val(TW_PORT, TW_SPD_PIN, !!value);
	for(i = 0; i < DELAY/2; i++);
	gpio_set_val(TW_PORT, TW_SCK_PIN, 1);
	for(i = 0; i < DELAY; i++);
}

/* To write: [6-bit address][1][X][8-bit data] */
static void write_register(u8 reg, u8 value)
{
	volatile int i;
	u16 data = ((reg<<(15-5)) | (1<<(15-6)) | (1<<(15-7)) | value);

	gpio_configure_pin(TW_PORT, TW_SPD_PIN, GPIO_GPIOFN, 1, 0, 0);

	/* idle clock low and enable the serial interface */
	gpio_set_val(TW_PORT, TW_SCK_PIN, 0);
	gpio_set_val(TW_PORT, TW_SPE_PIN, 0);
	for(i = 0; i < DELAY*2; i++);

	for (i = 0; i < 16; i++)
		tx(data & (1<<(15-i)));

	/* de-assert the serial interface enable signal */
	gpio_set_val(TW_PORT, TW_SPE_PIN, 1);
	for(i = 0; i < DELAY; i++);
}

void nt39016_init(void)
{
	/* initialize IO pins */
	gpio_configure_pin(TW_PORT, TW_SCK_PIN, GPIO_GPIOFN, 1, 0, 0);
	gpio_configure_pin(TW_PORT, TW_SPD_PIN, GPIO_GPIOFN, 1, 0, 0);
	gpio_configure_pin(TW_PORT, TW_SPE_PIN, GPIO_GPIOFN, 1, 0, 1);
	gpio_configure_pin(TW_PORT, TW_TEST_PIN, GPIO_GPIOFN, 0, 1, 0);

	/* write out the register configuration to program the device for our
	 * screen */
	write_register(0x0E, 0x68);
}
