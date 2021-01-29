/* hx8238.c - Detect and initialize the Himax LCD driver.
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdint.h>
#include <board.h>
#include <common.h>
#include <gpio.h>
#include <string.h>
#include <debug.h>
#include <spi.h>

static u16 hx8238_read(u8 reg)
{
	u32 res;

	spi_cs(0);
	spi_write24(0x70<<16 | reg);
	spi_cs(1);

	spi_cs(0);
	res = spi_write24(0x73<<16);
	spi_cs(1);

	return res;
}

static void hx8238_write(u16 reg, u16 value)
{
	spi_cs(0);
	spi_write24(0x70<<16 | reg);
	spi_cs(1);

	spi_cs(0);
	spi_write24(0x72<<16 | value);
	spi_cs(1);
}

/* Set up the SPI bus and try to detect the HX8238 driver.  The chip will only
 * respond if it detects its "address" and we know that, for example, register
 * 0x0005 always contains the value 0xB4C4 at startup. */
int hx8238_detect(void)
{
	u16 res;

	spi_init();

	res = hx8238_read(0x0005);
       	if (res == 0xB4C4)
		return 1;

	return 0;
}

/* Write the recommended settings to the HX8238 driver. */
void hx8238_init(void)
{
	hx8238_write(0x0004, 0x0447); /* SEL2-0: 000 (Parallel RGB format) */
	hx8238_write(0x0005, 0xBCC4);
}
