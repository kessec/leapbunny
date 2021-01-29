/* spi.c - Basic polling SPI driver
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
#include <common.h>
#include <base.h>
#include <gpio.h>
#include <board.h>
#include <string.h>
#include <debug.h>

/* SPI registers as offsets */
#define SSPSPICONT0	0x00
#define SSPSPICONT1	0x02
#define SSPSPIDATA	0x04
#define SSPSPISTATE	0x06
#define SSPSPICLKENB	0x40
#define SSPSPICLKGEN	0x44

/* CONT0 register */
#define ENB		   11
#define FFCLR		   10
#define NUMBIT		    5
#define DIVCNT		    0

/* CONT1 register */
#define SCLKPOL		    3
#define SCLKSH		    2
#define TYPE		    0

/* STATE register */
#define TXSHIFTEREMPTY      8
#define WFFEMPTY	    2
#define RFFEMPTY	    0

/* CLKENB register */
#define SPI_PCLKMODE	    3
#define SPI_CLKGENENB	    2

#define SPI8(x)		REG8(SPI0_BASE+x)
#define SPI16(x)	REG16(SPI0_BASE+x)
#define SPI32(x)	REG32(SPI0_BASE+x)

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
