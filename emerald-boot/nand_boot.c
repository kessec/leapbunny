/* nand_boot.c -- Reads remainder of bootstrap code from NAND to SDRam
 *
 * Copyright 2007-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Note: This driver is designed to enable booting from a large-block NAND 
 *       Flash on the LF1000 CPU, using hardware ECC.  Other Flash types and
 *       OOB layouts are not supported.
 */

#include "include/autoconf.h"
#include "include/board.h"
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/nand.h>
#include <mach/gpio.h>

#include "include/nand.h"
#include "include/debug.h"


/* assume: size is NAND page size aligned */
/* Only works for large-block devices!! */
/* SPEED: 175us/KB or ~22ms/128K erase block, 175ms/MB */ 

    // NOTE: this function is used only to read the rest of the bootstrap from
    //       nand when we're booting from nand, either the didj-ts base nand
    // or a nand on a cartridge that's jumpered as the base.
    //
    // This code does not check ECC to detect/correct errors.  
    // The code and data to do so does not fit into the first 2 KB page of a
    // NAND.  After all of emerald-boot.bin except the screens segment has
    // been read into ram, this code could start checking ECC and, where
    // possible, correcting errors.  Because of the code's limited use, we
    // have not bothered to do so.
    //
    // NOTE: This routine assumes that 'offset' is a multiple of the nand's
    //       page size.  It stores the entire contents of the page that contains
    // 'offset' at the beginning of the buffer 'buf'.  
    // This is a safe assumption, because the code is called from only 
    // nand_bootstrap().

int nand_read_bootstrap(u32 *buf, u32 offset, u32 u_size, u32 page_shift)
{
	u32 page = (offset >> page_shift); // page's index on the NAND device
	int i;
	long size = u_size;

	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */

	nand_init(&nand_cmd, &nand_addr, &nand_data);
	
	/* use the size for 32-bit chunks */
	size >>=2;

	do { /* read one page at a time */
		/* read the page's data */
		u32 end = (1 << (page_shift)) / sizeof(u32);
		nand_send_read_cmd(page, 0);
		nand_wait_for_ready();
            // first read the page's data section
		for(i = 0; i < end; i++) {
			*buf++ = NFDATA32();
			--size;
		}
       // since we can't check/correct, there's no reason to read the OOB
		++page;
	} while(size > 0);

	return 0;
}
