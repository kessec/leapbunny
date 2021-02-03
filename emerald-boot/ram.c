/* ram.c -- probe the ram available on board
 *
 * Copyright 2008-2011 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <board.h>
#include <common.h>
#include <string.h>

#define RAM_CMDLINE_SIZE 128
static char ram_cmdline[RAM_CMDLINE_SIZE];
static u32 ram_high_addr = 0;
static u32 mlc_fb_addr = 0;
static u32 mlc_fb_size = 0;

//static char LUT[] = "0123456789ABCDEF";
static const char LUT[] = "0123456789ABCDEF";

#if !defined BOARD_MLC_RESERVE_MEMORY
#warning Reserving 0 bytes for MLC frame buffer!
#define BOARD_MLC_RESERVE_MEMORY 0
#endif


#define SZ_1MB			(1024*1024)
#define TYPICAL_HIGH_ADDR	(32*SZ_1MB)

u32 get_frame_buffer_addr ()
{
	return mlc_fb_addr;
}

u32 get_frame_buffer_size ()
{
	return mlc_fb_size;
}

static void append_hex (u32 x)
{
	int i, l = strlen (ram_cmdline);
	ram_cmdline[l++] = '0';
	ram_cmdline[l++] = 'x';
	for (i=28; i>=0; i-=4)
		ram_cmdline[l++] = LUT[(x>>i) & 0xf];
	ram_cmdline[l] = 0;
}

// Return x/10; *r=x%10
u32 udivmod10 (u32 x, u32 *r)
{
	int q=0;
	while (x>=10)
		q++, x -= 10;
	*r = x;
	return q;
}

static void append_dec (int x)
{
	int l = strlen (ram_cmdline);
	if (x<0)
		x=-x, ram_cmdline[l++] = '-';
	int l0 = l;
	do {
		u32 q,r;
		q = udivmod10 (x, &r);
		ram_cmdline[l++] = '0' + r;
		x = q;
	} while (x);
	ram_cmdline[l--] = 0;
	// Reverse in place! Foo
	// If x=5, then l0=7, l=7
	// if x=15, then l0=7, l=8
	// if x=125, then l0=7, l=9
	char c;
	for (; l0<l; l0++, l--)
	{
		c=ram_cmdline[l0];
		ram_cmdline[l0]=ram_cmdline[l];
		ram_cmdline[l]=c;
	}
}	

#define	LOW_START	0x00000000
#define HIGH_START	0x80000000
#define MB(x)		((x)<<20)

// Fall back on reading some config registers

char *probe_ram ()
{
	u32 addr, size;

	volatile u32 *pNFCONTROL = (volatile u32 *)0xC0015874;

	u32 NFCONTROL = *pNFCONTROL;
	if (NFCONTROL & (1<<5))
	{
		// Booted from NAND or UART; so RAM at LOW address (0x0)
		addr = LOW_START;
	}
	else
	{
		// Booted from NOR RAM is at HIGH (0x8000_0000)
		addr = HIGH_START;
	}

	volatile u32 *pMCU_Y = (volatile u32 *)0xC0014800;

	u32 MCU_Y = *pMCU_Y;
	size = MB(8 << (MCU_Y & 3));

	// Save off top of RAM address
	ram_high_addr = addr + size;

	// Tell kernel how much MEM we have
	int mem = (size >> 20);
	u32 mlc_fb_size = 0;

	// Save this much RAM for FB off the top
	mem -= BOARD_MLC_RESERVE_MEMORY;
	mlc_fb_size = (BOARD_MLC_RESERVE_MEMORY)*SZ_1MB;
	mlc_fb_addr = ram_high_addr - mlc_fb_size;

	strcpy (ram_cmdline, "mem=");
	append_dec (mem);
	strcat (ram_cmdline, "M ");

	// Tell mlc driver where the sun shines
	strcat (ram_cmdline, "mlc_fb=");
	append_hex (mlc_fb_addr);
	strcat (ram_cmdline, ",");
	append_hex (mlc_fb_size);
	strcat (ram_cmdline," ");

	// Tell people where RAM is
	strcat (ram_cmdline, "ram=");
	append_hex (addr);
	strcat (ram_cmdline, "-");
	append_hex (addr+size-1);
	strcat (ram_cmdline, " ");
	return ram_cmdline;
}
