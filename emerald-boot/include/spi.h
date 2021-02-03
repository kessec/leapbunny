/* spi.h - Basic polling SPI driver
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SPI_H__
#define __SPI_H__

/* Initialize the SPI controller, set up pins, and reset its internal FIFO. */
void spi_init(void);

/* Drive the SPI chip select (SS#) pin to the value provided. */
void spi_cs(u8 value);

/* Perform a 16-bit transfer: write 'value' and return the shifted-in data */
u16 spi_write16(u16 value);

/* Perform a 24-bit transfer: write the lower 3 bytes of 'value' and return the
 * 3 bytes shifted in. */
u32 spi_write24(u32 value);

#endif /* __SPI_H__ */
