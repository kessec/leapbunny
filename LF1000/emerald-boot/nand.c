/* nand.c -- LF1000 NAND Controller Driver
 *
 * Copyright 2007-2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Note: This driver is designed to enable booting from a large-block NAND 
 *       Flash on the LF1000 CPU.  Other Flash types and OOB layouts are
 *       not supported.
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

    // TODO: FIXME: 11aug0
    //              main() calls nand_wake_bbt(), which queries the NAND
    // device(s) to determine the sizes of their erase blocks.  This function
    // assumes the base NAND erase block size is NAND_EB_SIZE.  Determine if
    // this function's assumption is valid.  If it is, maybe we can simplify
    // either nand_wake_bbt() or nand_get_erase_block_size().
    // If the assumption is not valid, change this function to query the NAND
    // device for the erase block size.

int nand_calculate_ecc(const unsigned char *buf, unsigned char *code);
int nand_correct_data(unsigned char *buf,
		              unsigned char *read_ecc, unsigned char *calc_ecc);

static void report_correctable_error( int page, int byteOffset, int bitIndex)
{
	serio_puts("\nCorrected bit "); serio_hex(bitIndex);
	serio_puts(" in byte 0x"); serio_int(byteOffset);
	serio_puts(" of page 0x"); serio_int(page); serio_putchar('\n');
}

static void report_uncorrectable_error( int page, int byteOffset, int size )
{
	serio_puts("Uncorrectable ECC error in NAND page 0x");
	serio_int( page );
	serio_puts(", bytes [0x"); serio_int(byteOffset);
	serio_puts(", 0x"); serio_int(byteOffset + size - 1);
	serio_puts("]\n");
}

static void report_error_in_ecc_bytes( int page, int byteOffset, int size )
{
	serio_puts("\nError in ECC data for bytes [0x"); serio_int(byteOffset);
	serio_puts(", 0x"); serio_int(byteOffset + size - 1);
	serio_puts("] of page "); serio_int(page);
	serio_putchar('\n');
}

static void report_stored_calcd_ecc( unsigned char * pStoredEcc,
                                     unsigned char * pCalcdEcc)
{
	serio_puts("  stored ecc: ");
	serio_byte(pStoredEcc[0]); serio_putchar(' ');
	serio_byte(pStoredEcc[1]); serio_putchar(' ');
	serio_byte(pStoredEcc[2]); serio_putchar('\n');
	serio_puts("  calc'd ecc: ");
	serio_byte(pCalcdEcc[0]);  serio_putchar(' ');
	serio_byte(pCalcdEcc[1]);  serio_putchar(' ');
	serio_byte(pCalcdEcc[2]);  serio_putchar('\n');
}

// returns 0 if ok (no error, correctable error, or ignorable error)
//        -1 if uncorrectable error
static int check_data( unsigned char * pStart,      // start of data
                       unsigned char * pStoredEcc,  // ECC read from flash
                       int             page,        // data's NAND page index
                       int             byteOffset,  // data's offset from start of page
                       int             dataSize )   // number of bytes of data
{
	unsigned char calcd_ecc[3];
	int           status;

	nand_calculate_ecc(pStart, calcd_ecc);
	if ((status = nand_correct_data(pStart, pStoredEcc, calcd_ecc))
	    < 0)
	{
		report_uncorrectable_error( page, byteOffset, dataSize);
		report_stored_calcd_ecc( pStoredEcc, calcd_ecc);
#if 1   // 30dec09
		{
			int i;
			int j;
			for (i = 0; i < dataSize; i += 16) {
				serio_byte(i);  serio_puts(": ");
				for (j = 0; j < 16; ++j) {
					serio_byte( *(pStart+i+j));  serio_putchar(' ');
				}
				serio_putchar('\n');
			}
		}
#endif
		return -1;
	}
	else if (status == 1) {
		// report the error and keep going
		report_error_in_ecc_bytes(page, byteOffset, dataSize);
	}
	else if (status >= 2)
	{
		// report the corrected error and keep going
		report_correctable_error( page, ((status & 0x0000FFFF) - 2) + byteOffset,
					  status >> 16);
	}
	// else no error detected (status == 0)
	return 0;
}

/* NOTE: This routine does not verify that the eraseblock in which 'offset'
 *       lies is a good eraseblock.  Before calling this function, the caller
 * ought to call nand_check_block() to see if the eraseblock is good.
 *
 * NOTE:
 * We effectively assume that the routine will be called to read data from only
 * one eraseblock.  If that assumption is not valid, we ought to change this
 * routine by adding code to check if subsequent eraseblocks are good and
 * by adding another parameter that indicates the maximum offset (from the
 * beginning of device) from which the routine ought to read data.
 *
 * NOTE: the routine assumes that u_size is a multiple of the nand's page size.
 *       The routine stores
 *          NAND_PAGE_SIZE * (u_size + NAND_PAGE_SIZE -1)/NAND_PAGE_SIZE
 *       bytes, starting at 'buf'.
 */
/* 1 bit ECC mode */
int nand_read1(u32 *buf, u32 offset, u32 u_size, struct nand_size_info *pInfo)
{
	u32 page = (offset >> pInfo->page_shift); // page's index on the NAND device
	u32 pageModulus = (page & pInfo->page_index_in_eb_mask);
	// page's index within erase block
	int pagesRead = 0;              // # of pages read by this routine
	u32 oob_buf[WORST_OOB_SIZE>>2];
	int i;
	int step;
	long size = u_size;

	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */

	unsigned char * pStart;
	unsigned char * pStoredEcc;

	db_puts("1-bit ECC\n");

	nand_init(&nand_cmd, &nand_addr, &nand_data);

	/* use the size for 32-bit chunks */
	size >>=2;

	do { /* read one page at a time */
		// Read page and oob with one command
		pStoredEcc = ((unsigned char *)&oob_buf[0]) + pInfo->ecc_offset;
		pStart     =  (unsigned char *)buf;

		/* read the page's data */
		nand_send_read_cmd(page, 0);
		nand_wait_for_ready();
		// first read the page's data section
		for(i = 0; i < pInfo->page_size/sizeof(u32); i++) {
			*buf++ = NFDATA32();
			--size;
		}
		// now read the page's OOB
		for(i = 0; i < (pInfo->oob_size>>2); i++)
			oob_buf[i] = NFDATA32();

		// NOTE: no check for bad block, because we assume caller has already
		//       verified that the block is good.

#if 0   // used during initial tests
		//if (step == 0)
		if (step == 3)
		{
			//    if (page == (offset >> NAND_PAGE_SHIFT))
			if (page == 7 + (offset >> pInfo->page_shift))
			{
				//pStart[0] ^= 8; /* flip bit 3 of byte 0 */
				pStart[17]  ^= 2; /* flip bit 1 of byte 0 */
				pStart[257] ^= 0x20; /* flip bit 5 of byte 257 */
				pStart[258] ^= 0x08; /* flip bit 3 of byte 258 */
				//serio_puts("Flipped byte 0, bit 3; byte 257, bit 5; byte 258, bit 3\n");
				serio_puts("Flipped byte 0x611, bit 1; byte 0x701, bit 5; byte 0x702, bit 3\n");
			}
			if (page == 2 + (offset >> pInfo->page_shift))
			{
				pStoredEcc[1] ^= 0x10;
				serio_puts("Flipped an ECC bit\n");
			}
			else serio_putchar('.');
		}
#endif

		/* Now check (and correct if possible) the page's data */
		for(step = 0; step < pInfo->ecc_steps; step++) {

			if (0 > check_data( pStart, pStoredEcc, page,
					    step * NAND_BCH_SIZE, 256))
			{
				return -1;
			}

			pStoredEcc += 3;
			pStart     += 256;
			if (0 > check_data( pStart, pStoredEcc, page,
					    256 + step * NAND_BCH_SIZE,
					    256))
			{
				return -1;
			}
			pStoredEcc += 3;
			pStart     += 256;
		}

		++pagesRead;
		++page;
		if(++pageModulus >= pInfo->pages_per_eb) {
			pageModulus = 0;
		}
	} while(size > 0);

	return 0;
}

int TryToCorrectBCH_Errors(u8 * pData, u8 * readECC) ;

/* 4 bit ECC mode */
int nand_read4(u32 *buf, u32 offset, u32 u_size, struct nand_size_info *pInfo)
{
	u32 page = (offset>>pInfo->page_shift);        // page's index on the NAND device
	u32 pageModulus = (page & ((pInfo->eb_size >> pInfo->page_shift) - 1));
	// page's index within erase block
	int pagesRead = 0;              // # of pages read by this routine
	u32 oob_buf[WORST_OOB_SIZE>>2];
	int i;
	int step;
	long size = u_size;
    u32 allFF;
    u32 curval;

	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */
	unsigned char * pStart;
	unsigned char * pStoredEcc;
	db_putchar('\n'); db_puts("+"); db_int(u_size); db_putchar(' ');

	db_puts("4-bit ECC\n");
	nand_init(&nand_cmd, &nand_addr, &nand_data);

	/* use the size for 32-bit chunks */
	size >>=2;

	do { /* read one page at a time */
		/* read the OOB for this page */
		nand_send_read_cmd(page, (u32)pInfo->page_size);
		nand_wait_for_ready();
		//		nand_command(CMD_READ);
		for(i = 0; i < (pInfo->oob_size>>2); i++)
			oob_buf[i] = NFDATA32();

		/* TODO: Fix this for MLC */
		if(pageModulus < 2 && (oob_buf[0] & 0xFF) != 0xFF) { /*bad block*/
			page += (pInfo->eb_size>>pInfo->page_shift) - pageModulus;   // move to 1st page of
			// next erase block
			/* if this is the second page of the block,
			 * and if we've already read a page, we must have read
			 * the first page of the block.  Since the block is bad,
			 * we ought to discard the block's first page.  To do so,
			 * subtract NAND_PAGE_SIZE from the buffer pointer.
			 */
			if (   (pageModulus == 1)
			       && (pagesRead != 0)) { /* discard the block's 1st page */
				buf  -= (pInfo->page_size / sizeof(u32)); // back up the pointer
				size += (pInfo->page_size / sizeof(u32)); // increase # to read
			}
			pageModulus = 0;
			continue;
		}

		// NOTE: this assumes the 28 bytes of 4-bit ECC start @ OOB[36]
		//pStoredEcc = ((unsigned char *)&oob_buf[0]) + 36;
		pStoredEcc = ((unsigned char *)&oob_buf[0]) + pInfo->ecc_offset;

		/* now read the page data in BCH-size chunks */
		for(step = 0; step < pInfo->ecc_steps; step++, pStoredEcc += 7) {

			pStart = (unsigned char *)buf;

			/* retrieve the ECC, store in ORGECC for BCH decoder */
			REG32(LF1000_MCU_S_BASE+NFORGECCL) =  pStoredEcc[0]
				+ ((u32)pStoredEcc[1] << 8)
				+ ((u32)pStoredEcc[2] << 16)
				+ ((u32)pStoredEcc[3] << 24);
			REG32(LF1000_MCU_S_BASE+NFORGECCH) =  pStoredEcc[4]
				+ ((u32)pStoredEcc[5] << 8)
				+ ((u32)pStoredEcc[6] << 16);
			/* read data from this chunk */
			nand_send_read_cmd(page, step*NAND_BCH_SIZE);
			nand_wait_for_ready();
            allFF = 0xFFFFFFFF;
			for(i = 0; i < (pInfo->page_size/pInfo->ecc_steps)/sizeof(u32); i++) {
				curval = NFDATA32();
                allFF &= curval;
				*buf++ = curval;
				--size;
			}

			/* wait for decoder to finish */
			while(IS_CLR(REG32(LF1000_MCU_S_BASE+NFECCSTATUS),
				     NFECCDECDONE));

			/* did decoder detect an error? */
			if(IS_SET(REG32(LF1000_MCU_S_BASE+NFECCSTATUS),
				  NFCHECKERROR))
			{   // if all the data was 0xFFFFFFFF, check if all ECC bytes are FF
                if (allFF == 0xFFFFFFFF) {  
                    for (i = 0; i < 7; ++i) {
                        if (pStoredEcc[i] != 0xFF)
                            break;
                    }
                }
                    // if at least one data byte or one ecc byte was not 0xFF
                    // call TryToCorrectBCH_Errors()
                if ( (allFF != 0xFFFFFFFF) || (i < 7))
                {
				    if (TryToCorrectBCH_Errors(pStart, pStoredEcc) < 0) {
					    int x;
					    serio_puts("BCH ECC errors\n");
					    {
						    int y;
						    serio_puts("; page "); serio_int(page);
						    serio_puts("; step "); serio_int(step); serio_puts("\n");
						    for (x = 0; x < NAND_BCH_SIZE; x += 16) {
							    serio_int(x); serio_puts(": ");
							    for (y = 0; y < 16; ++y) {
								    serio_byte(pStart[x+y]); serio_putchar(' ');
							    }
							    serio_putchar('\n');
						    }
						    serio_putchar('\n');
					    }
					    serio_puts("OOB: ");
					    for (x = 0; x < 7; ++x) {
						    serio_byte( pStoredEcc[x]); serio_putchar(' ');
					    }
					    serio_putchar('\n');
					    return -1;
				    }
                }
			}
		}   // step
		++pagesRead;
		++page;
		if(++pageModulus >= (pInfo->eb_size>>pInfo->page_shift))
			pageModulus = 0;
		db_putchar('.');

	} while(size > 0);

	db_putchar('\n');
	return 0;
}
