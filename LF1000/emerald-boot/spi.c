/* spi.c - Basic polling SPI driver
 *
 * Copyright 2007-2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
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
#include "include/spi_hal.h"

#define SPI8(x)		REG8(LF1000_SPI0_BASE+x)
#define SPI16(x)	REG16(LF1000_SPI0_BASE+x)
#define SPI32(x)	REG32(LF1000_SPI0_BASE+x)

/* We set the controller up for 8-bit trasfers even though it can do 16-bit so
 * that we can work with 24-bit transfer devices.  We configure the SS# pin as
 * a GPIO for the same reason. */
void spi_init(void)
{
	/* configure pins */
	gpio_configure_pin(GPIO_PORT_B, 14, GPIO_ALT1, 0, 0, 0); /* MISO */
	gpio_configure_pin(GPIO_PORT_B, 15, GPIO_ALT1, 1, 0, 0); /* MOSI */
	gpio_configure_pin(GPIO_PORT_B, 13, GPIO_ALT1, 1, 0, 0); /* SCK */
	gpio_configure_pin(GPIO_PORT_B, 12, GPIO_GPIOFN, 1, 0, 1); /* SS# */

	/* configure controller */
	SPI16(SSPSPICONT0) = (1<<ENB)|((8-1)<<NUMBIT)|(4<<DIVCNT);
	SPI16(SSPSPICONT1) = (0x01<<TYPE)|(0<<SCLKSH)|(1<<SCLKPOL);
	SPI16(SSPSPISTATE) = 0;
	SPI16(SSPSPISTATE) |= (1<<FFCLR);
	SPI32(SSPSPICLKENB) = (1<<SPI_PCLKMODE)|(1<<SPI_CLKGENENB);
	SPI16(SSPSPICLKGEN) = 0xE2;
}

void spi_cs(u8 value)
{
	gpio_set_val(GPIO_PORT_B, 12, value);
}

u16 spi_write16(u16 value)
{
	u16 res = 0;

	SPI8(SSPSPIDATA) = (value >> 8);
	while(!(SPI16(SSPSPISTATE) & (1<<WFFEMPTY)));
	while((SPI16(SSPSPISTATE) & (1<<RFFEMPTY)));
	res |= SPI8(SSPSPIDATA) << 8;

	SPI8(SSPSPIDATA) = (value >> 0);
	while(!(SPI16(SSPSPISTATE) & (1<<WFFEMPTY)));
	while((SPI16(SSPSPISTATE) & (1<<RFFEMPTY)));
	res |= SPI8(SSPSPIDATA) << 0;

	while(!(SPI16(SSPSPISTATE) & (1<<TXSHIFTEREMPTY)));

	return res;
}

u32 spi_write24(u32 value)
{
	u32 res = 0;

	SPI8(SSPSPIDATA) = (value >> 16);
	while(!(SPI16(SSPSPISTATE) & (1<<WFFEMPTY)));
	while((SPI16(SSPSPISTATE) & (1<<RFFEMPTY)));
	res |= SPI8(SSPSPIDATA) << 16;
	
	SPI8(SSPSPIDATA) = (value >> 8);
	while(!(SPI16(SSPSPISTATE) & (1<<WFFEMPTY)));
	while((SPI16(SSPSPISTATE) & (1<<RFFEMPTY)));
	res |= SPI8(SSPSPIDATA) << 8;

	SPI8(SSPSPIDATA) = (value >> 0);
	while(!(SPI16(SSPSPISTATE) & (1<<WFFEMPTY)));
	while((SPI16(SSPSPISTATE) & (1<<RFFEMPTY)));
	res |= SPI8(SSPSPIDATA) << 0;

	while(!(SPI16(SSPSPISTATE) & (1<<TXSHIFTEREMPTY)));

	return res;
}
