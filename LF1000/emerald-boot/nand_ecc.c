/* nand_ecc.c -- Routines and data for computing 1-bit ECC and correcting errors
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <board.h>
#include <common.h>
#include <base.h>
#include <nand.h>
#include <nand_controller.h>
#include <debug.h>


// Moved here from linux/drivers/mtd/nand/nand_ecc.c
typedef unsigned int uint32_t;
#define uninitialized_var(x) x

/*
 * invparity is a 256-byte table that contains the odd parity
 * for each byte. So if the number of bits in a byte is even,
 * the array element is 1, and when the number of bits is odd
 * the array element is 0.
 */
static const char invparity[256] = {
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

/*
 * bitsperbyte contains the number of bits per byte
 * this is only used for testing and repairing parity
 * (a precalculated value slightly improves performance)
 */
static const char bitsperbyte[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

/*
 * addressbits is a lookup table to filter out the bits from the xor-ed
 * ecc data that identify the faulty location.
 * this is only used for repairing parity
 * see the comments in nand_correct_data for more details
 */
static const char addressbits[256] = {
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
	0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
	0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
	0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
	0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
	0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
	0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
	0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
	0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
	0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
	0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
	0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
	0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
	0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
	0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
	0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f
};

/**
 * nand_calculate_ecc - Calculate 3-byte ECC for 256-byte block
 * @buf:	input buffer with raw data
 * @code:	address of buffer where the calculated 3 bytes of ECC will be stored
 */
int nand_calculate_ecc( const unsigned char *buf, unsigned char *code)
{
	int i;
	const uint32_t *bp = (uint32_t *)buf;
	const uint32_t eccsize_mult = 1;    /* assume ecc.size is 256 */
	uint32_t cur;		                /* current value in buffer */
	/* rp0..rp15..rp17 are the various accumulated parities (per byte) */
	uint32_t rp0, rp1, rp2, rp3, rp4, rp5, rp6, rp7;
	uint32_t rp8, rp9, rp10, rp11, rp12, rp13, rp14, rp15, rp16;
	uint32_t uninitialized_var(rp17);	/* to make compiler happy */
	uint32_t par;		/* the cumulative parity for all data */
	uint32_t tmppar;	/* the cumulative parity for this iteration;
				           for rp12, rp14 and rp16 at the end of the
				           loop */

	par  = 0;
	rp4  = 0;
	rp6  = 0;
	rp8  = 0;
	rp10 = 0;
	rp12 = 0;
	rp14 = 0;
	rp16 = 0;

	/*
	 * The loop is unrolled a number of times;
	 * This avoids if statements to decide on which rp value to update
	 * Also we process the data by longwords.
	 * Note: passing unaligned data might give a performance penalty.
	 * It is assumed that the buffers are aligned.
	 * tmppar is the cumulative sum of this iteration.
	 * needed for calculating rp12, rp14, rp16 and par
	 * also used as a performance improvement for rp6, rp8 and rp10
	 */
	for (i = 0; i < eccsize_mult << 2; i++) {
		cur     = *bp++;
		tmppar  = cur;
		rp4    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp6    ^= tmppar;
		cur     = *bp++;
		tmppar ^= cur;
		rp4    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp8    ^= tmppar;

		cur     = *bp++;
		tmppar ^= cur;
		rp4    ^= cur;
		rp6    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp6    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp4    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp10   ^= tmppar;

		cur     = *bp++;
		tmppar ^= cur;
		rp4    ^= cur;
		rp6    ^= cur;
		rp8    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp6    ^= cur;
		rp8    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp4    ^= cur;
		rp8    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp8    ^= cur;

		cur     = *bp++;
		tmppar ^= cur;
		rp4    ^= cur;
		rp6    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp6    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;
		rp4    ^= cur;
		cur     = *bp++;
		tmppar ^= cur;

		par    ^= tmppar;
		if ((i & 0x1) == 0)
			rp12 ^= tmppar;
		if ((i & 0x2) == 0)
			rp14 ^= tmppar;
		if ((eccsize_mult == 2) && ((i & 0x4) == 0))
			rp16 ^= tmppar;
	}

	/*
	 * handle the fact that we use longword operations
	 * we'll bring rp4..rp14..rp16 back to single byte entities by
	 * shifting and xoring first fold the upper and lower 16 bits,
	 * then the upper and lower 8 bits.
	 */
	rp4  ^= (rp4 >> 16);
	rp4  ^= (rp4 >> 8);
	rp4  &= 0xff;
	rp6  ^= (rp6 >> 16);
	rp6  ^= (rp6 >> 8);
	rp6  &= 0xff;
	rp8  ^= (rp8 >> 16);
	rp8  ^= (rp8 >> 8);
	rp8  &= 0xff;
	rp10 ^= (rp10 >> 16);
	rp10 ^= (rp10 >> 8);
	rp10 &= 0xff;
	rp12 ^= (rp12 >> 16);
	rp12 ^= (rp12 >> 8);
	rp12 &= 0xff;
	rp14 ^= (rp14 >> 16);
	rp14 ^= (rp14 >> 8);
	rp14 &= 0xff;
	if (eccsize_mult == 2) {
		rp16 ^= (rp16 >> 16);
		rp16 ^= (rp16 >> 8);
		rp16 &= 0xff;
	}

	/*
	 * we also need to calculate the row parity for rp0..rp3
	 * This is present in par, because par is now
	 * rp3 rp3 rp2 rp2 in little endian and
	 * rp2 rp2 rp3 rp3 in big endian
	 * as well as
	 * rp1 rp0 rp1 rp0 in little endian and
	 * rp0 rp1 rp0 rp1 in big endian
	 * First calculate rp2 and rp3
	 */
#ifdef __BIG_ENDIAN
	rp2  = (par >> 16);
	rp2 ^= (rp2 >> 8);
	rp2 &= 0xff;
	rp3  = par & 0xffff;
	rp3 ^= (rp3 >> 8);
	rp3 &= 0xff;
#else
	rp3  = (par >> 16);
	rp3 ^= (rp3 >> 8);
	rp3 &= 0xff;
	rp2  = par & 0xffff;
	rp2 ^= (rp2 >> 8);
	rp2 &= 0xff;
#endif

	/* reduce par to 16 bits then calculate rp1 and rp0 */
	par ^= (par >> 16);
#ifdef __BIG_ENDIAN
	rp0  = (par >> 8) & 0xff;
	rp1  = (par & 0xff);
#else
	rp1  = (par >> 8) & 0xff;
	rp0  = (par & 0xff);
#endif

	/* finally reduce par to 8 bits */
	par ^= (par >> 8);
	par &= 0xff;

	/*
	 * and calculate rp5..rp15..rp17
	 * note that par = rp4 ^ rp5 and due to the commutative property
	 * of the ^ operator we can say:
	 * rp5 = (par ^ rp4);
	 * The & 0xff seems superfluous, but benchmarking learned that
	 * leaving it out gives slightly worse results. No idea why, probably
	 * it has to do with the way the pipeline in pentium is organized.
	 */
	rp5  = (par ^ rp4) & 0xff;
	rp7  = (par ^ rp6) & 0xff;
	rp9  = (par ^ rp8) & 0xff;
	rp11 = (par ^ rp10) & 0xff;
	rp13 = (par ^ rp12) & 0xff;
	rp15 = (par ^ rp14) & 0xff;
	if (eccsize_mult == 2)
		rp17 = (par ^ rp16) & 0xff;

	/*
	 * Finally calculate the ecc bits.
	 * Again here it might seem that there are performance optimisations
	 * possible, but benchmarks showed that on the system this is developed
	 * the code below is the fastest
	 */
#ifdef BOARD_NAND_ECC_SMC
	code[0] =
	    (invparity[rp7] << 7) |
	    (invparity[rp6] << 6) |
	    (invparity[rp5] << 5) |
	    (invparity[rp4] << 4) |
	    (invparity[rp3] << 3) |
	    (invparity[rp2] << 2) |
	    (invparity[rp1] << 1) |
	    (invparity[rp0]);
	code[1] =
	    (invparity[rp15] << 7) |
	    (invparity[rp14] << 6) |
	    (invparity[rp13] << 5) |
	    (invparity[rp12] << 4) |
	    (invparity[rp11] << 3) |
	    (invparity[rp10] << 2) |
	    (invparity[rp9] << 1)  |
	    (invparity[rp8]);
#else
	code[1] =
	    (invparity[rp7] << 7) |
	    (invparity[rp6] << 6) |
	    (invparity[rp5] << 5) |
	    (invparity[rp4] << 4) |
	    (invparity[rp3] << 3) |
	    (invparity[rp2] << 2) |
	    (invparity[rp1] << 1) |
	    (invparity[rp0]);
	code[0] =
	    (invparity[rp15] << 7) |
	    (invparity[rp14] << 6) |
	    (invparity[rp13] << 5) |
	    (invparity[rp12] << 4) |
	    (invparity[rp11] << 3) |
	    (invparity[rp10] << 2) |
	    (invparity[rp9] << 1)  |
	    (invparity[rp8]);
#endif
	if (eccsize_mult == 1)
		code[2] =
		    (invparity[par & 0xf0] << 7) |
		    (invparity[par & 0x0f] << 6) |
		    (invparity[par & 0xcc] << 5) |
		    (invparity[par & 0x33] << 4) |
		    (invparity[par & 0xaa] << 3) |
		    (invparity[par & 0x55] << 2) |
		    3;
	else
		code[2] =
		    (invparity[par & 0xf0] << 7) |
		    (invparity[par & 0x0f] << 6) |
		    (invparity[par & 0xcc] << 5) |
		    (invparity[par & 0x33] << 4) |
		    (invparity[par & 0xaa] << 3) |
		    (invparity[par & 0x55] << 2) |
		    (invparity[rp17] << 1) |
		    (invparity[rp16] << 0);
	return 0;
}

/**
 * nand_correct_data - Detect and correct bit error(s)
 * @buf:	    raw data read from the chip
 * @read_ecc:	ECC from the chip
 * @calc_ecc:	the ECC calculated from raw data
 *
 * Detect and correct a 1 bit error for 256 byte block
 * 
 * Returns 0 if no error is detected
 *         1 if correctable error is detected in the ECC bytes
 *        (2 + N) if correctable error is detected in data byte N (N=0,1,...255)
 *                (the bit index is in the more significant word:
 *                   (bit << 16) +2 + N )
 *        -1 if uncorrectable errors are detected.
 */
int nand_correct_data( unsigned char *buf,
		               unsigned char *read_ecc, unsigned char *calc_ecc)
{
	unsigned char b0, b1, b2;
	unsigned char byte_addr, bit_addr;
	const uint32_t eccsize_mult = 1;    /* 256 bytes/ECC */

	/*
	 * b0 to b2 indicate which bit is faulty (if any)
	 * we might need the xor result  more than once,
	 * so keep them in a local var
	*/
#ifdef BOARD_NAND_ECC_SMC
	b0 = read_ecc[0] ^ calc_ecc[0];
	b1 = read_ecc[1] ^ calc_ecc[1];
#else
	b0 = read_ecc[1] ^ calc_ecc[1];
	b1 = read_ecc[0] ^ calc_ecc[0];
#endif
	b2 = read_ecc[2] ^ calc_ecc[2];

	/* check if there are any bitfaults */

	/* repeated if statements are slightly more efficient than switch ... */
	/* ordered in order of likelihood */

	if ((b0 | b1 | b2) == 0)
		return 0;	/* no error */

	if ((((b0 ^ (b0 >> 1)) & 0x55) == 0x55) &&
	    (((b1 ^ (b1 >> 1)) & 0x55) == 0x55) &&
	    ((eccsize_mult == 1 && ((b2 ^ (b2 >> 1)) & 0x54) == 0x54) ||
	     (eccsize_mult == 2 && ((b2 ^ (b2 >> 1)) & 0x55) == 0x55))) {
	/* single bit error */
		/*
		 * rp17/rp15/13/11/9/7/5/3/1 indicate which byte is the faulty
		 * byte, cp 5/3/1 indicate the faulty bit.
		 * A lookup table (called addressbits) is used to filter
		 * the bits from the byte they are in.
		 * A marginal optimisation is possible by having three
		 * different lookup tables.
		 * One as we have now (for b0), one for b2
		 * (that would avoid the >> 1), and one for b1 (with all values
		 * << 4). However it was felt that introducing two more tables
		 * hardly justify the gain.
		 *
		 * The b2 shift is there to get rid of the lowest two bits.
		 * We could also do addressbits[b2] >> 1 but for the
		 * performace it does not make any difference
		 */
		if (eccsize_mult == 1)
			byte_addr = (addressbits[b1] << 4) + addressbits[b0];
		else
			byte_addr = (addressbits[b2 & 0x3] << 8) +
				    (addressbits[b1] << 4) + addressbits[b0];
		bit_addr = addressbits[b2 >> 2];
		/* flip the bit */
		buf[byte_addr] ^= (1 << bit_addr);
		return (2 + byte_addr + (bit_addr << 16) );

	}
	/* count nr of bits; use table lookup, faster than calculating it */
	if ((bitsperbyte[b0] + bitsperbyte[b1] + bitsperbyte[b2]) == 1) {
		return 1;	/* error in ecc data; no action needed */
    }

	return -1;
}


// #else   // 4-bit ECC

/******************************************************************************
 * Lifted from Magic Eyes ./vtk/test/NAND_Flash/mes_nand.c
 *
//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//  MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
 *
 * MES_NAND_GetErrorLocation( int *pLocation )
 ******************************************************************************/

#include "bch_tables.c"

//------------------------------------------------------------------------------
// Derived from Magic Eyes's
// int MES_NAND_GetErrorLocation( int *pLocation )
//
int lf1000_GetErrorLocation (int *pLocation, u16 s1, u16 s3, u16 s5, u16 s7)
{
    register int i, j, elp_sum ;
    int count;
    int r;          // Iteration steps
    int Delta;      // Discrepancy value
    int elp[8+1][8+2];  // Error locator polynomial (ELP)
    int L[8+1];     // Degree of ELP
    int B[8+1][8+2];    // Scratch polynomial
    //  int root[8+1];  // Roots of ELP
    int reg[8+1];       // Register state
    int s[8];

    s[0] = s1;
    s[2] = s3;
    s[4] = s5;
    s[6] = s7;

    // Even syndrome = (Odd syndrome) ** 2
    for( i=1,j=0 ; i<8 ; i+=2, j++ ) {
        if( s[j] == 0 ) s[i] = 0;
        else            s[i] = BCH_AlphaToTable[ (2 * BCH_IndexOfTable[s[j]])
                                                % 8191];
    }

    // Initialization of elp, B and register
    for( i=0 ; i<=8 ; i++ ) {
        L[i] = 0 ;
        for( j=0 ; j<=8 ; j++ ) {
            elp[i][j] = 0 ;
            B[i][j] = 0 ;
        }
    }

    for( i=0 ; i<=4 ; i++ ) {
        reg[i] = 0 ;
    }

    elp[1][0] = 1 ;
    elp[1][1] = s[0] ;

    L[1] = 1 ;
    if( s[0] != 0 )
        B[1][0] = BCH_AlphaToTable[(8191 - BCH_IndexOfTable[s[0]]) % 8191];
    else
        B[1][0] = 0;

    for( r=3 ; r<=8-1 ; r=r+2 ) {
        // Compute discrepancy
        Delta = s[r-1] ;
        for( i=1 ; i<=L[r-2] ; i++ ) {
            if( (s[r-i-1] != 0) && (elp[r-2][i] != 0) )
                Delta ^= BCH_AlphaToTable[(BCH_IndexOfTable[s[r-i-1]] 
                                           + BCH_IndexOfTable[elp[r-2][i]])
                                          % 8191];
        }
        if( Delta == 0 ) {
            L[r] = L[r-2] ;
            for( i=0 ; i<=L[r-2] ; i++ ) {
                elp[r][i] = elp[r-2][i];
                B[r][i+2] = B[r-2][i] ;
            }
        }
        else {
            // Form new error locator polynomial
            for( i=0 ; i<=L[r-2] ; i++ ) {
                elp[r][i] = elp[r-2][i] ;
            }
            for( i=0 ; i<=L[r-2] ; i++ ) {
                if( B[r-2][i] != 0 )
                    elp[r][i+2] ^= BCH_AlphaToTable[ (BCH_IndexOfTable[Delta] 
                                                + BCH_IndexOfTable[B[r-2][i]]) 
                                                    % 8191];
            }
            // Form new scratch polynomial and register length
            if( 2 * L[r-2] >= r ) {
                L[r] = L[r-2] ;
                for( i=0 ; i<=L[r-2] ; i++ ) {
                    B[r][i+2] = B[r-2][i];
                }
            }
            else {
                L[r] = r - L[r-2];
                for( i=0 ; i<=L[r-2] ; i++ ) {
                    if( elp[r-2][i] != 0 )
                        B[r][i] = BCH_AlphaToTable[
                                            (BCH_IndexOfTable[elp[r-2][i]]
                                             + 8191 - BCH_IndexOfTable[Delta]) 
                                                    % 8191];
                    else
                        B[r][i] = 0;
                }
            }
        }
    }

    if( L[8-1] > 4 ) {
        return -1;
    }
    else {
        // Chien's search to find roots of the error location polynomial
        // Ref: L&C pp.216, Fig.6.1
        for( i=1 ; i<=L[8-1] ; i++ )
            reg[i] = elp[8-1][i];

        count = 0;
        for( i=1 ; i<=8191 ; i++ ) {
            elp_sum = 1;
            for( j=1 ; j<=L[8-1] ; j++ ) {
                if( reg[j] != 0 ) {
                    reg[j] = BCH_AlphaToTable[(BCH_IndexOfTable[reg[j]] + j)
                                              % 8191] ;
                    elp_sum ^= reg[j] ;
                }
            }
            if( !elp_sum ) {    // store root and error location number indices
                // Convert error location from systematic form to storage form
                pLocation[count] = 8191 - i;

                if (pLocation[count] >= 52) {
                    // Data Bit Error
                    pLocation[count] = pLocation[count] - 52;
                    pLocation[count] = 4095 - pLocation[count];
                }
                else {
                    // ECC Error
                    pLocation[count] = pLocation[count] + 4096;
                }
                if( pLocation[count] < 0 )  
                    return -1;

                count++;
            }
        }

        if( count == L[8-1] ) { // # of roots = degree of elp hence <= 4 errors
            return L[8-1];
        }
        else { // Number of roots != degree of ELP => >4 errors and cannot solve
            return -1;
        }
    }
}



    /* returns 0 if no errors
     *      N >0 if N errors were corrected
     *        <0 if uncorrectable errors
     */
int TryToCorrectBCH_Errors(u8 * pData, u8 * readECC) 
{
    u32 x, s7, s5, s3, s1;
    int errorLocations[4];
    int numErrors;
    int eccbytes = 7;
    int eccsize  = 512;

        /* Pull out the 4 syndrome words */
			

    x  = REG32(MCU_S_BASE+NFSYNDRONE75);
    s7 = (x >> SYNDROM7) & 0x1fff;
    s5 = (x >> SYNDROM5) & 0x1fff;

    x  = REG32(MCU_S_BASE+NFSYNDRONE31);
    s3 = (x >> SYNDROM3) & 0x1fff;
    s1 = (x >> SYNDROM1) & 0x1fff;

    numErrors = lf1000_GetErrorLocation( &errorLocations[0], s1, s3, s5, s7);
    if (numErrors < 0)  /* uncorrectable errors */
    {
		/* Check if the section and its ECC area are completely
		 * erased.  If they are, don't report an uncorrectable
		 * error.
		 */
		int eccSubIndex;
		int dataIndex;
		
		for (eccSubIndex = 0; eccSubIndex < eccbytes; ++eccSubIndex)
		{
			if (readECC[eccSubIndex] != 0xFF)
		    	break;
		}
			/*  check the data only if all ECC bytes are 0xFF */
		if (eccSubIndex >= eccbytes)
		{								  
			for (dataIndex = 0; dataIndex < eccsize; ++dataIndex)
			{
				if (pData[dataIndex] != 0xFF)
					break;
			}
	        /* If all ECC bytes and all data bytes are 0xFF, 
             * there's no error.  The page section is erased and unwritten.
		     */
		    if (dataIndex >= eccsize)
		    {
	            numErrors = 0;
		    }
		}
    }
	    /* If there is one or more correctable errors 		 
         * ('numErrors' is the number of correctable errors)
	     */
    else if (numErrors > 0) {
        int errorOffset;
        int errorMask;
        int j;

        for (j = 0; j < numErrors; ++j)
        {
            errorOffset = errorLocations[j] >> 3;
            errorMask   = 1 << (errorLocations[j] & 7);
            serio_puts("Flip bit "); serio_int(errorLocations[j]);
            serio_puts("; offset "); serio_int(errorOffset);
            serio_puts("; mask ");   serio_byte(errorMask);
            serio_puts("\n");
		        /* We see errorOffset >= 512 when an ECC bit is in error
		    	 * Don't bother correcting ECC bit errors, because the
				 * bit positions might be beyond the end of the buffer.
				 */
			if (errorOffset < eccsize) {
            //  TODO: FIXME: test error detection/correction.
               	*(pData + errorOffset) ^= errorMask;
			}
        }
    }
    return numErrors;
}

