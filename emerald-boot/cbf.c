/* cbf.c -- access to Common Boot Format, a simple packing format for binaries.
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef DEBUGTEST
typedef unsigned long u32;
typedef unsigned char u8;
#include <string.h>
#include <stdio.h>
void db_int (u32 x) { printf ("%08lx", x); }
void db_puts (char *s) { printf ("%s", s); }
u8 cruft [2*1024*1024];
#else
#include <common.h>
#include <nand.h>
#include <debug.h>
#include <global.h>
#include <string.h>
#include <cbf.h>
#endif


/* Structure is as follows:
	u32 magic;
	u32 version
	u32 load_address -- relative to start of RAM
	u32 jump_address  -- relative to start of RAM
	u32 num_bytes -- no assumption on rounding
	u32 crc_header -- header only, up to but not including this field
	data -- kernel image + root fs if any )
	u32 crc_data -- data only
*/

struct cbf_hdr {
	u32 magic;
	u32 version;
	u32 load_address;
	u32 jump_address;
	u32 num_bytes;
	u32 crc_header;
} __attribute__((__packed__));

struct cbf_tail {
	u32 crc_data;
} __attribute__((__packed__));

/* cbf singleton object
 * Two public methods:
 *   cbf_init - call once
 *   cbf_process_chunk - Processes arbitrary chunk of input stream in
 *      one pass, depositing data in load_address (if validated).
 *      Maintains state internally.
 *	Returns OK, error or more-data-needed
 */

enum cbf_state { INHEADER, INDATA, INTAIL, DONE, ERROR };

struct cbf_class {
	enum cbf_state state;
	u32 count;
	void *ram_base;
	struct cbf_hdr hdr;
	struct cbf_tail tail;
} cbf_space;

inline struct cbf_class *get_cbf(void)
{
#if 1	// 30mar11
//#define NOR_RELOC_OFFSET	0x600000	
#define NOR_RELOC_OFFSET	0x800000	
#if 1	// 1apr11
	u32 rambase = calc_SDRAM_ADDRESS();
	if (rambase == 0x00000000) {	// If booted from NAND, don't alter addre
		return &cbf_space;
	}
		// if booted from NOR and copied to RAM, adjust for RAM address and
		// for offset within RAM
	return ((struct cbf_class *)(rambase + NOR_RELOC_OFFSET + (u32)&cbf_space));
#else
	u32 ram_base = NOR_RELOC_OFFSET + calc_SDRAM_ADDRESS();
	return (struct cbf_class *)(ram_base + (u8*)&cbf_space);
#endif	// 1apr11

#else	// repo
	u32 ram_base = calc_SDRAM_ADDRESS();
	return (struct cbf_class *)(ram_base + (u8*)&cbf_space);
#endif
}

/* Prototypes for forward reference */
int cbf_validate_header ();
int cbf_finish_data ();
int cbf_validate_data ();
int cbf_common (u8 *buf, u32 len, u32 need, u8 *where, int (*next)(void),
		u8 **suggested_buf);

/*
 * Public functions
 */

/* cbf_init ()
 * Call once to set up cbf object
 */
void cbf_init ()
{
	struct cbf_class *cbf = get_cbf();

	// Relocate this global
	cbf->state = INHEADER;
	cbf->count = 0;
	cbf->ram_base = (void *)calc_SDRAM_ADDRESS();

	db_puts ("cbf_init ram_base=");
	db_int ((unsigned int)(cbf->ram_base));
	db_puts ("\n");
}

/* 
 * cbf_get_data_size()
 *	If the cbf processing state indicates that the size of the cbf package is
 *	known, return the number of bytes in the package (sizeof(header) + number
 *	of data bytes + sizeof(tail)).
 *	Otherwise return -1.
 */
int cbf_get_data_size() {
	int size;
	struct cbf_class *cbf = get_cbf();

	switch (cbf->state) {
	case INDATA:
	case INTAIL:
	case DONE:
		size = cbf->hdr.num_bytes + sizeof(struct cbf_hdr)
								  + sizeof(struct cbf_tail);
		break;

	default:
		size = -1;
		break;
	}
	return size;
}

/* cbf_get_jump_address (void **jump)
 * Return 0 for success or -1 for error in processing file
 * Return jump address (corrected with ram_base) in *jump
 */
int cbf_get_jump_address (void **jump)
{
	struct cbf_class *cbf = get_cbf();

	if (cbf->state == DONE)
	{
		*jump = cbf->hdr.jump_address + (char *)cbf->ram_base;
		return 0;
	}
	return -1;
}

/* cbf_get_status ()
 * Return 0 for Done and OK, 1 for more data needed, and -1 for error 
 */
int cbf_get_status ()
{
	return cbf_process_chunk (NULL, 0, NULL);
}

/* Process a chunk of data
 * Place data in load_address if header has been received and validated
 * Otherwise, place data into our own statically allocated header cbf->hdr
 * Return -1 for error (and db_puts string)
 * Return 0 for done
 * Return 1 for more data needed
 * Also return a suggested buffer address if suggested_buf!=NULL.
 * If you use this address as the buf for the next call to cbf_process_chunk,
 * then cbf_process_chunk will not have to memcpy to its final destination.
 */
int cbf_process_chunk (u8 *buf, u32 len, u8 **suggested_buf)
{
	struct cbf_class *cbf = get_cbf();

	// db_puts ("cbf_process_chunk: len="); db_int (len); 
	// db_puts (" buf="); db_int (buf);
	// db_puts (" cbf="); db_int (cbf);
	// db_puts (" count="); db_int (cbf->count);
	switch (cbf->state)
	{
	default:
			serio_puts("XXXXXXXXXXXXXXXXXXXXXX\n");
			serio_int(cbf->state);
			serio_putchar('\n');
	case ERROR:
		serio_puts ("CBF: ERROR\n");
		return -1;
	case INHEADER:
		db_puts ("INHEADER ");
		return cbf_common (buf, len, sizeof(cbf->hdr) - cbf->count,
				   (u8 *)&cbf->hdr + cbf->count,
				   cbf_validate_header, suggested_buf);
	case INDATA:
#ifndef DEBUG_VERBOSE
		if (len > cbf->hdr.num_bytes - cbf->count)
#endif
			db_puts ("INDATA ");
		return cbf_common (buf, len, cbf->hdr.num_bytes - cbf->count,
#ifdef DEBUGTEST
				   (u8 *)cruft + cbf->count,
#else
				   (u8 *)cbf->ram_base +
				   	cbf->hdr.load_address + cbf->count, 
#endif
				   cbf_finish_data, suggested_buf);
	case INTAIL:
		db_puts ("INTAIL ");
		return cbf_common (buf, len, sizeof(cbf->tail) - cbf->count,
				   (u8 *)&cbf->tail + cbf->count,
				   cbf_validate_data, suggested_buf);
	case DONE:
		db_puts ("DONE ");
		return 0;
	}
}

static inline void memcpy32(u32 *d, u32 *s, u32 len)
{
	len >>= 2;
	while (len--)
		*d++ = *s++;
}

static inline void memcpy16(u16 *d, u16 *s, u32 len)
{
	len >>= 1;
	while (len--)
		*d++ = *s++;
}


/* Worker
 * Return -1 for error (and db_puts string)
 * Return 0 for done
 * Return 1 for more data needed
 */
#ifdef DEBUG_VERBOSE
#define TEST 	1
#else
#define TEST 	(len > need)
#endif
int cbf_common (u8 *buf, u32 len, u32 need, u8 *where, int (*next)(void),
		u8 **suggested_buf)
{
	struct cbf_class *cbf = get_cbf();
	int accept = len;
	int ret=1; // by default, we want more input

	if (TEST)
	{
		db_puts ("cbf_common: buf=");
		db_int ((unsigned int)buf);
		db_puts (" len=");
		db_int (len);
		db_puts (" need=");
		db_int (need);
		db_puts (" where=");
		db_int ((unsigned int)where);
	}
	if (len <= 0)
	{
		// Handle special case of hitting end of header exactly
		//  by suggesting next time we send buf=where
		if (cbf->state == INDATA)
		{
			if (TEST)
			{
				db_puts (" suggest=");
				db_int ((unsigned int)where);
				db_puts ("\n");
			}
			if (suggested_buf)
				*suggested_buf = where;
		}
		return ret;
	}
	if (accept > need)
		accept = need;
	if (buf != where)
	{
           // If both 'where' and 'buf' are on 4-byte boundaries, use memcpy32
           // If both 'where' and 'buf' are on 2-byte boundaries, use memcpy16
           // Otherwise, use memcpy
        if (   (0 == (0x03 & (u32)where))
            && (0 == (0x03 & (u32)buf)))
        {
		    memcpy32 ((u32 *)where, (u32 *)buf, accept);
        }
        else if (   (0 == (0x01 & (u32)where))
                 && (0 == (0x01 & (u32)buf)))
        {
		    memcpy16 ((u16 *)where, (u16 *)buf, accept);
        }
        else {
            memcpy((char *)where, (char *)buf, accept);
        }
		if (TEST)
		{
			db_puts (" Memcpy=");
			db_int (accept);
		}
	}
	cbf->count += accept;
	if (suggested_buf)
		*suggested_buf = where + accept;
	if (TEST)
	{
		db_puts (" suggest=");
		db_int ((unsigned int)(where+accept));
		db_puts ("\n");
	}
	if (accept == need)
	{
		// Done; update state, count
		ret = (*next)();
		if (!ret)
		{
			// (*next)() returned OK; process rest, if any
			return cbf_process_chunk (buf+accept, len-accept,
						  suggested_buf);
		}
	}
	return ret;
}

#if 0	// moved to an assembly language routine in startup.S
/* Compute simple checksum
 * Pass in Incoming either 0 or checksum from previous block
 */
u32 cbf_checksum (u8 *buf, u32 len, u32 incoming)
{
	// printf ("crc len=%d\n", len);
	u32 *buf32 = (u32 *)buf;
	len >>= 2;
	u32 cs = incoming;

		/* while there are at least 8 more words to process, process 8 words
		 * on each pass through this loop.
		 * NOTE: we could experiment with different "unrolling" factors,
		 *		 e.g., 4, 12, 16, 20, 24.
		 */
	while (len >= 8)  
	{
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		cs = 1 + (cs ^ *buf32++);
		len -= 8;
	}
		/* fewer than 8 more words to process.  Process one per pass. */
	while (len--)
	{
		cs = 1 + (cs ^ *buf32++);
	}
	return cs;
}
#endif	// 4apr11

/*
 * Finish accepting data
 * Return error state directly: -1 for error, 0 for OK
 */
int cbf_finish_data ()
{
	struct cbf_class *cbf = get_cbf();

	// Done; update state, count
	cbf->state = INTAIL;
	cbf->count = 0;
	return 0;
}

/*
 * Validate the header; update state, count.  
 * Return error state directly: -1 for error, 0 for OK
 */
int cbf_validate_header ()
{
	struct cbf_class *cbf = get_cbf();
	u32 crc;

	cbf->count = 0;
	if (cbf->hdr.magic != 0x9abcdef0)
	{
		serio_puts ("cbf_validate_header: magic wrong:");
		serio_int (cbf->hdr.magic);
		serio_puts ("\n");
		cbf->state = ERROR;
		return -1;
	}
	if (cbf->hdr.version != 1)
	{
		serio_puts ("cbf_validate_header: magic version:");
		serio_int (cbf->hdr.version);
		serio_puts ("\n");
		cbf->state = ERROR;
		return -1;
	}

	/* validate checksum */
	crc = cbf_checksum ((u8 *)&cbf->hdr, 
			    (u32)((char *)&cbf->hdr.crc_header - (char *)&cbf->hdr),
			    0);
	if (crc != cbf->hdr.crc_header)
	{
		serio_puts ("cbf_validate_header: checksum wrong.  Found: ");
		serio_int (crc);
		serio_puts (" expected: ");
		serio_int (cbf->hdr.crc_header);
		serio_puts ("\n");
		cbf->state = ERROR;
		return -1;
	}
	/* All's well */
	cbf->state = INDATA;
	return 0;
}

/*
 * Validate the data; update state, count.  
 * Return error state directly: -1 for error, 0 for OK
 */
int cbf_validate_data ()
{
	struct cbf_class *cbf = get_cbf();
	u32 crc;

	cbf->count = 0;
#ifdef DEBUGTEST
	crc = cbf_checksum ((u8 *)cruft, cbf->hdr.num_bytes, 0);
#else
	crc = cbf_checksum ((u8 *)cbf->ram_base + cbf->hdr.load_address, 
			    cbf->hdr.num_bytes, 0);
#endif
	if (crc != cbf->tail.crc_data)
	{
		serio_puts ("cbf_validate_data: checksum wrong.  Found: ");
		serio_int (crc);
		serio_puts (" expected: ");
		serio_int (cbf->tail.crc_data);
		serio_puts (".  Computed over ");
		serio_int (cbf->hdr.num_bytes);
		serio_puts (" bytes\n");
		cbf->state = ERROR;
		return -1;
	}
	/* All's well */
	cbf->state = DONE;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUGTEST
int main (int c, char **v)
{
	long t;
	time (&t);
	srand (t);
	FILE *f;
	static u8 buf[65536], *b;
	f = fopen ("../scripts/kernel.cbf", "r");
	if (!f)
		exit (1);
	cbf_init ((void *)0x80000000);
	b=buf;
	int l=24;
	while (!feof (f))
	{
		int x = fread (b, 1, l, f);
		if (x < 0)
			exit (2);
		cbf_process_chunk (b, x, &b);
		printf ("b=%08x: ", b);
		l = 65536; // rand () % 512;
	}
	exit (0);
}

#endif
