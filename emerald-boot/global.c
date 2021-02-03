/* global.c -- global variable access
 *
 * Copyright 2011 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <base.h>
#include <common.h>
#include <global.h>
#include <nand_controller.h>

/* determine RAM address based on LF1000 shadow bit setting */
u32 calc_SDRAM_ADDRESS(void)
{
	/* NOR BOOT, shadow disabled */
	if (IS_CLR(REG32(MCU_S_BASE + NFCONTROL), NFBOOTENB))
		return 0x80000000;

	/* NAND BOOT, shadow enabled */
	return 0x00000000;
}

/* global program data */
global_var global;

global_var * get_global(void) {
#define NOR_RELOC_OFFSET	0x800000	
	u32 rambase = calc_SDRAM_ADDRESS();
	if (rambase == 0x00000000) {	// If booted from NAND, don't alter addre
		return &global;
	}
		// if booted from NOR and copied to RAM, adjust for RAM address and
		// for offset within RAM
	return ((global_var *)(rambase + NOR_RELOC_OFFSET + (u32)&global));
}


/* initialize global data. */
void init_global(void) {
	u32 * gp32 = (u32 *)get_global();

	for (int i = 0; i < sizeof(global)/sizeof(u32); i++)
		*gp32++ = 0;
}


