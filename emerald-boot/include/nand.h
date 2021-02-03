/* nand.h  -- NAND controler HAL and NAND access functionality
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __NAND_H__
#define __NAND_H__

/* NAND Flash Commands */
#define CMD_READ                0x00
#define CMD_READ_CONFIRM        0x30    /* (large block Flash chips only) */
#define CMD_READ_STATUS         0x70
#define NAND_CMD_READID         0x90
#define NAND_CMD_RESET          0xff

/* NAND status codes */
#define NAND_STATUS_ERROR_SH	0
#define NAND_STATUS_READY_SH	6
#define NAND_STATUS_ERROR	(1<<NAND_STATUS_ERROR_SH) //0x01
#define NAND_STATUS_READY	(1<<NAND_STATUS_READY_SH) //0x40

/* NAND flags */
#define NAND_LARGE_BLOCK_SH     0
#define NAND_EXTRA_ADDRESS_SH   1

/*------------------------------------------------------------------------------
 * Enable the following #define to include code that marks kernel partition
 * blocks bad and that erases all kernel partition blocks, even if they are
 * marked bad.  
 * Blocks are marked bad when LS, RS, and Dpad-Up are pressed at power-up.
 * Blocks are erased when LS, RS, and Dpad-Down are pressed at power-up.
 */
//#define TEST_BOOT_WITH_KERNEL_BAD_BLOCKS    1
/*----------------------------------------------------------------------------*/

/*
 * settings:
 *	the NAND controller BCH handles 512 bytes at a time
 *	64-byte OOB, with ECC stored in the end
 */

// FIXME: Plenty of room for packing, if memory gets dear
struct nand_size_info
{
	// Fundamental Values
	unsigned long eb_size;
	unsigned long page_size;	// Could be unsigned short?
	unsigned long oob_size;		// Could be unsigned short?
	unsigned long ecc_bytes;	// Could be unsigned char?
	int (*nand_read)(u32 *buf, u32 offset, u32 size, struct nand_size_info *pInfo);
	int (*nand_check_block)(u32 offset, struct nand_size_info *pInfo);

	// Derived Values
	unsigned long eb_shift;		// Could be unsigned char?
	unsigned long page_shift;	// Could be unsigned char?
	unsigned long eb_mask;
	unsigned long page_mask;	// Could be unsigned short?

	unsigned long pages_per_eb;	// Could be unsigned short?
	unsigned long page_index_in_eb_mask;

	unsigned long ecc_steps;	// Could be unsigned char?
	unsigned long ecc_offset;	// Could be unsigned short?
};

#define WORST_EB_SIZE	(512*1024)	// Our 2G MLC
#define WORST_OOB_SIZE	(256)		// 2x bigger than 2G MLC

#define NAND_BCH_SIZE	        512 /* this is dictated by LF1000 hdwe */
#define NAND_BCH_SHIFT	        9	/* this is dictated by LF1000 hdwe */

/*  NAND controller access */
#define nand_command(cmd)	(REG8(nand_cmd) = cmd)
#define nand_address(addr)	(REG8(nand_addr) = addr)
#define NFDATA8()		     REG8( nand_data)
#define NFDATA16()		     REG16(nand_data)
#define NFDATA32()		     REG32(nand_data)

int  nand_check_block_other(u32 offset, struct nand_size_info *pInfo);
int  nand_check_block_sansung_mlc(u32 offset, struct nand_size_info *pInfo);
void nand_init(unsigned int * pnand_cmd, 
                      unsigned int * pnand_addr, 
                      unsigned int * pnand_data);
void nand_send_read_cmd(u32 page, u32 column);
void nand_wait_for_ready(void);
int  nand_wake_bbt (struct nand_size_info *pInfo);

int nand_read0(u32 *buf, u32 offset, u32 size, struct nand_size_info *pInfo);
int nand_read1(u32 *buf, u32 offset, u32 size, struct nand_size_info *pInfo);
int nand_read4(u32 *buf, u32 offset, u32 size, struct nand_size_info *pInfo);
int nand_read_bootstrap(u32 *buf, u32 offset, u32 size, u32 page_shift);

#endif /* __NAND_H__ */
