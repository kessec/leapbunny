/* vim: set ts=4 sw=4 noexpandtab
 *
 * Copyright (C) 2011 Jeff Kent <jakent@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <asm/arch/gpio.h>
#include <asm/arch/gpio_hal.h>
#include <asm/arch/uart.h>
#include "uart.h"
#include "common.h"

baudinfo_t baudinfo[] = {
	BAUDINFO(  19200, 1, 39, 11),
	BAUDINFO(  38400, 1, 39,  5),
	BAUDINFO(  57600, 1, 39,  3),
	BAUDINFO( 115200, 1, 39,  1),
	BAUDINFO( 230400, 1,  4,  7),
	BAUDINFO( 460800, 1,  4,  3),
	BAUDINFO( 614400, 1,  4,  2),
	BAUDINFO( 921600, 1,  4,  1),
	BAUDINFO(1500000, 0, 15,  0),
	END_BAUDINFO,
};

/*
 * This is a minimal UART initialization.  We assume that PLL1 is set to the
 * default 147.461538 MHz and most of the UART registers are sane.  Tested
 * on the POLLUX VR3520F.
 */
void init_uart(u32 baudrate)
{
	baudinfo_t *p = find_baudinfo(baudrate);
	if (!p) {
		return;
	}

	/* Wait until the transmitter is empty */
	while (IS_CLR(REG16(LF1000_SYS_UART_BASE+TRSTATUS),
	              TRANSMITTER_EMPTY));

	/* Configure clock */
	REG16(LF1000_SYS_UART_BASE+UARTCLKGEN) = p->clkgen;
	/* Set baudrate divisor */
	REG16(LF1000_SYS_UART_BASE+BRD) = p->brd;
	/* Set GPIO as TX */
	REG32(LF1000_GPIO_BASE+GPIOAALTFN0) = GPIO_ALT1 << (GPIO_PIN8*2);
	/* Enable TX and RX */
	REG16(LF1000_SYS_UART_BASE+UCON) = (1<<TRANS_MODE)|(1<<RECEIVE_MODE);
}

baudinfo_t *find_baudinfo(u32 baudrate)
{
	baudinfo_t *p = &baudinfo[0];

	while (p->baudrate) {
		if (p->baudrate == baudrate) {
			return p;
		}
		p++;
	}
	return NULL;
}

u8 getc(void)
{
	while (IS_CLR(REG16(LF1000_SYS_UART_BASE+TRSTATUS),
	              RECEIVE_BUFFER_DATA_READY));
	return REG8(LF1000_SYS_UART_BASE+RHB);
}

void putc(u8 c)
{
	while (IS_CLR(REG16(LF1000_SYS_UART_BASE+TRSTATUS),
	              TRANSMIT_BUFFER_EMPTY));
	REG8(LF1000_SYS_UART_BASE+THB) = c;
}

u16 get_u16(void)
{
	u16 n;
	n = getc();
	n |= getc() << 8;
	return n;
}

void put_u16(u16 n)
{
	putc(n);
	putc(n >> 8);
}

u32 get_u32(void)
{
	u32 n;
	n = get_u16();
	n |= get_u16() << 16;
	return n;
}

void put_u32(u32 n)
{
	put_u16(n);
	put_u16(n >> 16);
}

