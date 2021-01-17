/* nand_utils.c -- LF1000 NAND Controller Driver
 *
 * Copyright 2007-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Note: This driver is designed to enable booting from a large-block
 *       NAND Flash on the LF1000 CPU.  Other Flash types and OOB layouts 
 *       are not supported.
 */

#include "include/autoconf.h"
#include "include/board.h"
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/nand.h>
#include <mach/gpio.h>

#include "include/nand.h"
#include "include/debug.h"

#define NAND_CMD_READID	0x90
#define NAND_CMD_RESET  0xff


/* calculate nand register address */
void nand_init(unsigned int * pnand_cmd, 
               unsigned int * pnand_addr, 
               unsigned int * pnand_data)
{
	if (IS_CLR(REG32(LF1000_MCU_S_BASE + NFCONTROL), NFBOOTENB)) {
		/* NOR BOOT, shadow disabled */
		*pnand_cmd  = LF1000_STATIC11_BASE_LOW + NFCMD;
		*pnand_addr = LF1000_STATIC11_BASE_LOW + NFADDR;
		*pnand_data = LF1000_STATIC11_BASE_LOW + NFDATA;
	} else {
		/* NAND BOOT, shadow enabled */
		*pnand_cmd  = LF1000_STATIC11_BASE_HIGH + NFCMD;
		*pnand_addr = LF1000_STATIC11_BASE_HIGH + NFADDR;
		*pnand_data = LF1000_STATIC11_BASE_HIGH + NFDATA;
	}
}

void nand_send_read_cmd(u32 page, u32 column)
{
	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */

	nand_init(&nand_cmd, &nand_addr, &nand_data);

	nand_command(CMD_READ);
	nand_address(column>>0);
	nand_address(column>>8);
	nand_address(page>>0);
	nand_address(page>>8);
	nand_address(page>>16);
	nand_command(CMD_READ_CONFIRM);
}

void nand_wait_for_ready(void)
{
	while(IS_CLR(REG32(LF1000_MCU_S_BASE+NFCONTROL), RnB));
}


/* nand_check_block()
 * 
 * Checks if the block that contains the specified offset from the start of
 * the NAND device is marked as a bad block.
 * 
 * The routine assumes that a bad block has a non-FF value in the first
 * byte of the OOB (spare area) of the block's first or second pages.
 *
 * TODO: FIXME:
 * NOTE: The Micron datasheet for MT29F4G08AAC and MT29F4G08ABC says the
 *       first byte of the OOB of the first page of a bad block will be
 * zero.  It says nothing about the second page.
 */
int nand_check_block_other(u32 offset, struct nand_size_info *pInfo)
{
	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */
	int          i;

	nand_init(&nand_cmd, &nand_addr, &nand_data);

	offset  &= pInfo->eb_mask;    /* offset of 1st byte of eraseblock */
	offset >>= pInfo->page_shift; /* Shift to get the page index */

	for(i = 0; i < 2; i++) {
		nand_send_read_cmd((u32)(offset + i), (u32)pInfo->page_size);
		nand_wait_for_ready();
		if ((NFDATA8() & 0xFF) != 0xFF) {
			return 1;
		}
	}
	return 0;
}

// nand is MLC; bad block is indicated by first byte of OOB of last page
int nand_check_block_samsung_mlc(u32 offset, struct nand_size_info *pInfo)
{
	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */

	nand_init(&nand_cmd, &nand_addr, &nand_data);

	offset  &= pInfo->eb_mask;    /* offset of 1st byte of eraseblock */
	offset  += pInfo->eb_size - pInfo->page_size; /* offset of block's last page */
	offset >>= pInfo->page_shift; /* Shift to get the page index */

	nand_send_read_cmd((u32)offset, (u32)pInfo->page_size); // read from OOB
	nand_wait_for_ready();
	if ((NFDATA8() & 0xFF) != 0xFF) {
		return 1;
	}
	return 0;
}

#ifdef DEBUG_NAND_ID
char mybuf[1024]; int mbx=0;
char lut[16] = "0123456789abcdef";
#endif

/* return EBS_64, 128, 256, or 512: the size of erase block in bytes
   selected nand */
static void nand_get_erase_block_size (struct nand_size_info *pInfo)
{
	u8 third, fourth;
	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */
	int mlc;

	nand_init(&nand_cmd, &nand_addr, &nand_data);
	
	// Good stuff is in the 4th id byte
	nand_command(NAND_CMD_READID);
	// Write 0 address
	nand_address(0);
#ifdef DEBUG_NAND_ID
	u8 x;
	x = NFDATA8(); // mfg
	mybuf[mbx++]=lut[x>>4]; mybuf[mbx++]=lut[x&0xf]; mybuf[mbx++]=',';
	x = NFDATA8(); // device
	mybuf[mbx++]=lut[x>>4]; mybuf[mbx++]=lut[x&0xf]; mybuf[mbx++]=',';
	x = third = NFDATA8(); // cell type, # of chips, etc
	mybuf[mbx++]=lut[x>>4]; mybuf[mbx++]=lut[x&0xf]; mybuf[mbx++]=',';
	x= fourth = NFDATA8(); // erase block size in bits 4,5; 0=64, 1=128, etc.
	mybuf[mbx++]=lut[x>>4]; mybuf[mbx++]=lut[x&0xf]; mybuf[mbx++]=';';
#else
	NFDATA8(); // mfg
	NFDATA8(); // device
	third = NFDATA8(); // cell type, # of chips, etc
	fourth = NFDATA8(); // erase block size in bits 4,5; 0=64, 1=128, etc.
#endif
	if (fourth == 0xff)
		fourth = 0;
	pInfo->page_size = 1024 << (fourth & 3);
	pInfo->oob_size = (8 << ((fourth >> 2) & 1)) * (pInfo->page_size >> 9);
	pInfo->eb_size = EBS_64 << ((fourth >> 4) & 3);

	// Decide the ECC: SLC=6, MLC=7
	mlc = ((third >> 2) & 3) != 0;
	pInfo->ecc_bytes = mlc ? 7 : 6;
	pInfo->nand_read = mlc ? nand_read4 : nand_read1;
	// FIXME: Should check_block depend on chip ID?  That is fragile and may change
	pInfo->nand_check_block = mlc ? nand_check_block_samsung_mlc : nand_check_block_other;

	// Now, compute derivatives
	pInfo->eb_shift = 16 + ((fourth >> 4) & 3);
	pInfo->eb_mask = ~(pInfo->eb_size - 1);
	pInfo->page_shift = 10 + (fourth & 3);
	pInfo->page_mask = ~(pInfo->page_size - 1);
	pInfo->pages_per_eb = pInfo->eb_size >> pInfo->page_shift;
	pInfo->page_index_in_eb_mask = pInfo->pages_per_eb - 1;

	pInfo->ecc_offset = pInfo->oob_size - pInfo->ecc_bytes * (pInfo->page_size>>NAND_BCH_SHIFT);
	pInfo->ecc_steps = pInfo->page_size>>NAND_BCH_SHIFT;
}

/* Work-around for Micron 512MB flash forgetting it's Bad-block marks */
int nand_wake_bbt (struct nand_size_info *pInfo)
{
	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */

	nand_init(&nand_cmd, &nand_addr, &nand_data);

	u32 tmp;

#ifdef DEBUG_NAND_ID
	mbx=0;
#endif

	/* Select cart */
	tmp = REG32(LF1000_MCU_S_BASE+NFCONTROL);
	BIT_SET(tmp, NFBANK);
	REG32(LF1000_MCU_S_BASE+NFCONTROL)=tmp;

	/* This works to wake up BBT in micron parts */
	nand_command (NAND_CMD_RESET);
	nand_wait_for_ready ();

	/* Select base */
	tmp = REG32(LF1000_MCU_S_BASE+NFCONTROL);
	BIT_CLR(tmp, NFBANK);
	REG32(LF1000_MCU_S_BASE+NFCONTROL)=tmp;

	/* Do it for base, in case we're booting from ATAP */
	nand_command (NAND_CMD_RESET);
	nand_wait_for_ready ();

	/* Get the erase block size */
	nand_get_erase_block_size (pInfo);

#ifdef DEBUG_NAND_ID
	mybuf[mbx++]=' ';
	mybuf[mbx++]=0;
#endif
	return 0;
}

#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS   // 28dec09pm
    // force one block of the nand to be bad
#define CMD_SERIAL_DATA_IN  0x80
#define CMD_PAGE_PROGRAM    0x10
volatile unsigned int vint;

void force_bad_block_in_nand(unsigned int address) {
	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */
	unsigned int page;          // index (on the device) of the first page
                                // of the eraseblock

	page  = (address >> NAND_PAGE_SHIFT); // page's index on the NAND device
    page &= ~NAND_PAGE_INDEX_IN_EB_MASK;  // index of the eraseblock's 1st page
#ifdef NAND_IS_MLC  // get index of block's last page
    page += (NAND_PAGES_PER_EB - 1);
#endif

	nand_init(&nand_cmd, &nand_addr, &nand_data);

	nand_command(CMD_SERIAL_DATA_IN);
	nand_address(NAND_PAGE_SIZE>>0);
	nand_address(NAND_PAGE_SIZE>>8);
	nand_address(page>>0);
	nand_address(page>>8);
	nand_address(page>>16);
    NFDATA8() = 0;          // write a 0 to 1st byte of page's OOB
	nand_command(CMD_PAGE_PROGRAM);

        // Delay to avoid premature detection of ready
    for (page = 0; page < 10000; ++page) {
        vint ^= page;
    }
    nand_wait_for_ready();
}

    // erase one block in nand, even if marked bad
#define CMD_ERASE_SETUP     0x60
#define CMD_ERASE_CONFIRM   0xD0

void erase_block_in_nand(unsigned int address) {
	unsigned int nand_cmd;		/* address of nand controller CMD register */
	unsigned int nand_addr;     /*                            ADDR         */
	unsigned int nand_data;     /*                            DATA         */
	unsigned int page;          // index (on the device) of the first page
                                // of the eraseblock

	page  = (address >> NAND_PAGE_SHIFT); // page's index on the NAND device
    page &= ~NAND_PAGE_INDEX_IN_EB_MASK;  // index of the eraseblock's 1st page

	nand_init(&nand_cmd, &nand_addr, &nand_data);

	nand_command(CMD_ERASE_SETUP);
	nand_address(page>>0);
	nand_address(page>>8);
	nand_address(page>>16);
	nand_command(CMD_ERASE_CONFIRM);

        // Delay to avoid premature detection of ready
    for (page = 0; page < 10000; ++page) {
        vint ^= page;
    }
    nand_wait_for_ready();
}
#endif  // 28dec09pm

