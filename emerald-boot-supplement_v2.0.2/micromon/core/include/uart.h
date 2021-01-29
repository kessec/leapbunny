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
 
#ifndef __UART_H__
#define __UART_H__

#include <asm/arch/common.h>
#include <asm/arch/uart.h>

typedef struct {
	u32 baudrate;
	u16 clkgen;
	u16 brd;
} baudinfo_t;

#define DEFAULT_BAUD 19200

#define BAUDINFO(baud, clksrc, div, brd) \
	{baud, (div << UARTCLKDIV) | (clksrc << UARTCLKSRCSEL), brd}
#define END_BAUDINFO {0, 0, 0}

void init_uart(u32 baudrate);
baudinfo_t *find_baudinfo(u32 baudrate);
u8 getc(void);
void putc(u8 c);
u16 get_u16(void);
void put_u16(u16 n);
u32 get_u32(void);
void put_u32(u32 n);

#endif /* __UART_H__ */
