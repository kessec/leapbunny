/* copyNand.c -- wrapper for nand_read() to enable self-bootstrapping 
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <nand.h>
#include <debug.h>

#ifdef SELF_BOOTSTRAP
/* Bootstrap settings: these are determined by the hardware.  
 * The LF1000 CPU's NAND Boot state machine loads 0x800 (one Large Block
 * NAND page) of data for us.  We need to load the rest,
 * which is _copy_end - _copy_start minus what's already loaded. 
 */
#define BOOTSTRAP_SIZE	0x0800
u32 GetCopySize();  // Returns the value of _copy_end.
#endif

void nand_bootstrap()
{
	const u32 page_size = 2048;
	const u32 page_shift = 11;

	/* Note: Turn off this option if you are testing emerald-boot by
	 *       forcing it into address 0 via u-boot or some other means like
	 *       JTAG. */
#ifdef SELF_BOOTSTRAP
	/* load the rest of the boot module (hardware loaded 2K) */
	// Call nand_read_bootstrap() to read (u32)_copy_end - BOOTSTRAP_SIZE
	// (rounded up to the nearest multiple of the page size) bytes
	// and store them in SDRam, starting at address BOOTSTRAP_SIZE.
	nand_read_bootstrap((u32 *)BOOTSTRAP_SIZE, BOOTSTRAP_SIZE, 
			(GetCopySize() - BOOTSTRAP_SIZE + page_size - 1)
			& ~(page_size - 1),
			page_shift);
#endif
	/* we're in SDRAM, now can call all functions */
	db_init();

#ifdef SELF_BOOTSTRAP
	db_puts("GetCopySize(): "); db_int( GetCopySize() ); 
	db_puts("; 3rd arg: "); 
	db_int( (GetCopySize() - BOOTSTRAP_SIZE + page_size - 1)
			& ~((page_size - 1)) ); 
#endif
	db_putchar('\n');
}
