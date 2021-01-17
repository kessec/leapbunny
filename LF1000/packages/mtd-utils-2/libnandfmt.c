#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include "mtd-utils/ubi-utils/new-utils/include/libmtd.h"
#include "mtd-utils/ubi-utils/new-utils/include/libubi.h"
#include "mtd-utils/ubi-utils/new-utils/include/libscan.h"
#include "mtd-utils/ubi-utils/new-utils/src/crc32.h"

#include <mtd_swab.h>
#include <mtd/mtd-user.h>
#include <mtd/ubi-header.h>

#include "libnandfmt.h"

// From linux-2.6/drivers/mtd/ubi/ubi-media.h
typedef unsigned long __be32;
typedef unsigned char __u8;
typedef long long __be64;
typedef long long loff_t;

/* /\* New globals to support retrying writes and removing fork/exec code *\/ */
/* static int gMTDfd; */
/* static unsigned char *gpBuf; */
/* static unsigned long gEBSize; */
/* static loff_t gFlashOffset, gFlashEndOffset; */
/* static unsigned long gEBUsed; */

static int nandfmt_mark_bad (struct mtd_info *mtd, unsigned long i)
{
#if defined(FAKE)
	printf ("Pretending to do nandfmt_mark_bad %u\n", i);
#else
	loff_t offset = (loff_t)i * mtd->eb_size;
	int x=ioctl (mtd->fd, MEMSETBADBLOCK, &offset);
	if (x)
	{
		printf ("ioctl MEMSETBADBLOCK failed with %d on %u\n", 
			x, (unsigned)i);
	}
#endif
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

#if 0
static void dump (char *buf)
{
	int i, j, count=0;
	printf (" ====\n");
	for (i=0; i<128*1024; i+=16)
	{
		int any=0;
		const char *b = &buf[i];
		for (j=0; j<16; j++)
			if (b[j] != 0xff)
				any=1;
		if (any && ++count < 16)
		{
			printf ("  %06x:", i);
			for (j=0; j<16; j++)
				printf (" %02x", b[j]);
			printf ("\n");
		}
	}
	printf ("Count=%d lines of non-ffs\n", count);
}
#endif

//////////////////////////////////////////////////////////////////////////////

// DFTP-style streaming access

static struct nandfmt_info {
	struct mtd_info mtd;
	struct ubi_scan_info *si;
	unsigned long current_i, end_i;
	unsigned long vid_hdr_offs, ec_hdr_write_size, data_offs;
	unsigned long offset;
	int in_ubi_mode;
	int verbose;
	char *buf;
} nandfmt_info;

// Only return when we've succeeded in writing a block, or run out of blocks.
// Update I to reflect what block is to be used next
// Return 0 for OK, non-zero for trouble
static int flash_a_block (const char *buf, int format_only)
{
	int err, try;
	long long ec;
	int write_len;
 next:
	if (nandfmt_info.current_i >= nandfmt_info.end_i)
	{
		// Special case, formatting and the last block is bad
		// is not a problem
		if (format_only)
			return 0;
		printf ("Ran out of room! Failing format.\n");
		return 1;
	}

	if (nandfmt_info.in_ubi_mode)
	{
		// Get erase counter from NAND
		ec = nandfmt_info.si->ec[nandfmt_info.current_i];
		// Skip bad blocks
		if (ec == EB_BAD)
		{
			printf ("Skipping bad block %u\n", nandfmt_info.current_i);
			nandfmt_info.current_i++;
			goto next;
		}

		// Make up a count if we don't have one
		if (ec < 0 || ec > EC_MAX)
			ec = nandfmt_info.si->mean_ec;

		// Compute write length, pruning FF's off the end of the buffer
		int i;
		for (i=nandfmt_info.mtd.eb_size-1; i>=0; i--)
			if (((const unsigned char *)buf)[i] != 0xff)
				break;
		write_len = (i + 1 + nandfmt_info.mtd.min_io_size - 1);
		write_len /= nandfmt_info.mtd.min_io_size;
		write_len *= nandfmt_info.mtd.min_io_size;
		if (nandfmt_info.verbose)
			printf ("write len=%d\n", write_len);
	}
	else
	{
		ec = 0;
		write_len = nandfmt_info.mtd.eb_size;
	}

	try = 0;
 again:
	// Erase
	// Check for bad block by reading from NAND if not UBI mode...
	if (!nandfmt_info.in_ubi_mode)
	{
		loff_t offset = (loff_t) nandfmt_info.current_i * 
			nandfmt_info.mtd.eb_size;
		if (ioctl (nandfmt_info.mtd.fd, MEMGETBADBLOCK, &offset) > 0)
		{
			printf ("Skipping bad block %u\n", nandfmt_info.current_i);
			nandfmt_info.current_i++;
			goto next;
		}
	}

	/* Erase */
	erase_info_t erase;
	erase.start = nandfmt_info.current_i * nandfmt_info.mtd.eb_size;
	erase.length = nandfmt_info.mtd.eb_size;
#if defined(FAKE)
	printf ("Pretending to do ioctl(MEMERASE %u)\n", nandfmt_info.current_i);
	if (!(rand() & 15))
#else
	if (ioctl (nandfmt_info.mtd.fd, MEMERASE, &erase) != 0)
#endif
	{
		printf ("Block failed to erase, marking block %u bad\n", nandfmt_info.current_i);
		nandfmt_mark_bad (&nandfmt_info.mtd, nandfmt_info.current_i);
		if (nandfmt_info.in_ubi_mode)
			nandfmt_info.si->ec[nandfmt_info.current_i] = EB_BAD;
		nandfmt_info.current_i++;
		goto next;
	}
	else
	{
		// Success, go on and try writing
		ec++;
	}

	// Early out if we don't need to write
	if (format_only && !nandfmt_info.in_ubi_mode)
	{
		// Success, bump count
		if (nandfmt_info.verbose)
			printf ("Erased only i=%u=%x\n", 
				nandfmt_info.current_i, nandfmt_info.current_i);
		nandfmt_info.current_i++;
		return 0;
	}

	// Write
	if (nandfmt_info.verbose)
		printf ("Writing i=%u=%x ec=%d len=0x%x\n", 
			nandfmt_info.current_i, nandfmt_info.current_i, (int) ec, write_len);

#if defined(FAKE)
	printf ("Pretending to do nandfmt_write (%u, ec=%d)\n", nandfmt_info.current_i, (int)ec);
	if (rand() & 1)
		// retry
#else
        if (nandfmt_info.in_ubi_mode)
	{
		// Make a header
		struct ubi_ec_hdr *ech = (struct ubi_ec_hdr *)buf;
		// Paint new erase counter over old header
		ech->ec = cpu_to_be64(ec);
		// Recompute the CRC and paint it back in
		unsigned long crc = crc32(UBI_CRC32_INIT, ech, UBI_EC_HDR_SIZE_CRC);
		ech->hdr_crc = cpu_to_be32(crc);
	}

	// Write to NAND
	unsigned long offset = (unsigned long)nandfmt_info.current_i * 
		nandfmt_info.mtd.eb_size;
	if (pwrite (nandfmt_info.mtd.fd, buf, write_len, offset) != write_len)
		// retry
#endif
	{
		printf ("Write try %d failed on block %u: ", ++try, nandfmt_info.current_i);
		if (try < 3)
		{
			printf ("Retrying\n");
			goto again;
		}
		printf ("Marking block %u bad\n", nandfmt_info.current_i);
		nandfmt_mark_bad (&nandfmt_info.mtd, nandfmt_info.current_i);
		if (nandfmt_info.in_ubi_mode)
			nandfmt_info.si->ec[nandfmt_info.current_i] = EB_BAD;
		nandfmt_info.current_i++;
		goto next;
	}

	// Success!  Go to next block
	nandfmt_info.current_i++;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int nandfmt_open (const char *dev, unsigned long start, unsigned long length,
		 int in_ubi_mode, int verbose)
{
	int err;

	memset ((char *)&nandfmt_info.mtd, 0, sizeof(nandfmt_info.mtd));
	nandfmt_info.verbose = verbose;
	nandfmt_info.in_ubi_mode = in_ubi_mode;
	err = mtd_get_info (dev, &nandfmt_info.mtd);
	if (err)
	{
		printf ("Can't mtd_get_info on %s\n", dev);
		return 1;
	}
	if (nandfmt_info.verbose)
	{
		printf ("size=%08x\n", (unsigned long)nandfmt_info.mtd.size);
		printf ("eb_cnt=%d\n", nandfmt_info.mtd.eb_cnt);
		printf ("eb_size=%d\n", nandfmt_info.mtd.eb_size);
		printf ("min_io_size=%d\n", nandfmt_info.mtd.min_io_size);
		printf ("subpage_size=%d\n", nandfmt_info.mtd.subpage_size);
	}
	nandfmt_info.buf = malloc (nandfmt_info.mtd.eb_size);
	if (!nandfmt_info.buf)
	{
		printf ("Can't malloc %d\n", nandfmt_info.mtd.eb_size);
		return 1;
	}

	err = ubi_scan (&nandfmt_info.mtd, &nandfmt_info.si, 0);
	if (err)
	{
		printf ("Can't ubi_scan\n");
		return 1;
	}

	if (nandfmt_info.si->vid_hdr_offs == -1 || nandfmt_info.si->data_offs == -1)
	{
		// Ug, could not find these, so compute them now
		// Probably because nand was erased and no previously formatted blocks were found
		nandfmt_info.si->vid_hdr_offs = (UBI_EC_HDR_SIZE + 
					 nandfmt_info.mtd.min_io_size - 1);
		
		nandfmt_info.si->vid_hdr_offs /= nandfmt_info.mtd.min_io_size;
		nandfmt_info.si->vid_hdr_offs *= nandfmt_info.mtd.min_io_size;

		nandfmt_info.si->data_offs = (nandfmt_info.si->vid_hdr_offs +
				      UBI_VID_HDR_SIZE + 
				      nandfmt_info.mtd.min_io_size - 1);
		nandfmt_info.si->data_offs /= nandfmt_info.mtd.min_io_size;
		nandfmt_info.si->data_offs *= nandfmt_info.mtd.min_io_size;
		printf ("vid_hdr_offs and data_offs not found; computing\n");
	}

	if (nandfmt_info.verbose)
	{
		printf ("mean_ec=%d\n", (unsigned long) nandfmt_info.si->mean_ec);
		printf ("ok_cnt=%d\n", nandfmt_info.si->ok_cnt);
		printf ("empty_cnt=%d\n", nandfmt_info.si->empty_cnt);
		printf ("corrupted_cnt=%d\n", nandfmt_info.si->corrupted_cnt);
		printf ("alien_cnt=%d\n", nandfmt_info.si->alien_cnt);
		printf ("bad_cnt=%d\n", nandfmt_info.si->bad_cnt);
		printf ("good_cnt=%d\n", nandfmt_info.si->good_cnt);
		printf ("vid_hdr_offs=%d\n", nandfmt_info.si->vid_hdr_offs);
		printf ("data_offs=%d\n", nandfmt_info.si->data_offs);
	}

	nandfmt_info.vid_hdr_offs = nandfmt_info.si->vid_hdr_offs;
	nandfmt_info.ec_hdr_write_size = nandfmt_info.si->vid_hdr_offs;
	nandfmt_info.data_offs = nandfmt_info.si->data_offs;

	// Compute starting point and ending point
	nandfmt_info.current_i = start;
	nandfmt_info.current_i /= nandfmt_info.mtd.eb_size;
	if (nandfmt_info.current_i * nandfmt_info.mtd.eb_size != start)
	{
		printf ("Starting point %08x not on erase block boundary.  Exiting\n", start);
		return 1;
	}
	if (length == 0)
		nandfmt_info.end_i = nandfmt_info.mtd.eb_cnt;
	else
	{
		nandfmt_info.end_i = start + length;
		nandfmt_info.end_i /= nandfmt_info.mtd.eb_size;
		if (nandfmt_info.end_i * nandfmt_info.mtd.eb_size != start + length)
		{
			printf ("Ending point %08x not on erase block boundary.  Exiting\n", start+length);
			return 1;
		}
		// Clip end to chip size, since last partition may be spec'd larger than possible
		if (nandfmt_info.end_i > nandfmt_info.mtd.eb_cnt)
		{
			nandfmt_info.end_i = nandfmt_info.mtd.eb_cnt;
			printf ("Clipping ending point to end of chip: %08x\n",
				(unsigned long) nandfmt_info.end_i * nandfmt_info.mtd.eb_size);
		}
	}
	
	if (nandfmt_info.verbose)
	{
		printf ("start_i=%x\n", nandfmt_info.current_i);
		printf ("  end_i=%x\n", nandfmt_info.end_i);
		printf ("in_ubi_mode=%d\n", nandfmt_info.in_ubi_mode);
	}

	return 0;
}

static void nandfmt_free ()
{
	free (nandfmt_info.buf);
	ubi_scan_free (nandfmt_info.si);
	close (nandfmt_info.mtd.fd);
}

int nandfmt_close (void (*pBusyCallback)(int percent))
{
	// Flush out partial buffers
	if (nandfmt_info.offset > 0)
	{
		if (nandfmt_info.verbose)
			printf ("+++++++++++++++++++++\n");
		flash_a_block (nandfmt_info.buf, 0);
		nandfmt_info.offset = 0;
	}
	// Wipe buffer except for header
	memset (nandfmt_info.buf + nandfmt_info.ec_hdr_write_size, 0xff,
		nandfmt_info.mtd.eb_size - nandfmt_info.ec_hdr_write_size);

	// Format a header
	if (nandfmt_info.in_ubi_mode)
	{
		struct ubi_ec_hdr *ech = (struct ubi_ec_hdr *)nandfmt_info.buf;
		memset(ech, '\0', sizeof(struct ubi_ec_hdr));
		ech->magic = cpu_to_be32(UBI_EC_HDR_MAGIC);
		ech->version = 1; // ui->ubi_ver;
		// ech->ec = cpu_to_be64(ec);
		ech->vid_hdr_offset = cpu_to_be32(nandfmt_info.vid_hdr_offs);
		ech->data_offset = cpu_to_be32(nandfmt_info.data_offs);
		// crc = crc32(UBI_CRC32_INIT, hdr, UBI_EC_HDR_SIZE_CRC);
		// ech->hdr_crc = cpu_to_be32(crc);
	}
		
	// Format to the end of the device
	long i=0, end=nandfmt_info.end_i - nandfmt_info.current_i;
	while (nandfmt_info.current_i < nandfmt_info.end_i)
	{
		// once in a while...call the busy callback
		if ((++i & 15)==0 && pBusyCallback)
			(*pBusyCallback)(100*i/end);
		if (nandfmt_info.verbose)
			printf ("=====================\n");
		flash_a_block (nandfmt_info.buf, 1);
	}

	// Clean up
	nandfmt_free ();
	return 0;
}

// Process an arbitrary chunk of data
int nandfmt_write (char *buf, int length)
{
	unsigned long i = 0;
	int try;
	unsigned long consume;
	int amt;

	while (length > 0)
	{
		consume = nandfmt_info.mtd.eb_size - nandfmt_info.offset;
		amt = length;
		// printf ("nandfmt_write: buf=%x len=%d consume=%d...", 
		//	(unsigned long)buf, length, consume);
		if (amt < consume)
		{
			// Take partial data and return
			memcpy (nandfmt_info.buf + nandfmt_info.offset, buf, amt);
			nandfmt_info.offset += amt;
			// printf ("offset=%d done\n", nandfmt_info.offset);
			return 0;
		}
		else
		{
			// Take as much as a block, and then check for more
			amt = consume;
			memcpy (nandfmt_info.buf + nandfmt_info.offset, buf, amt);
			// printf ("amt=%d...\n", amt);
			if (nandfmt_info.verbose)
				printf ("---------------------\n");
			if (flash_a_block (nandfmt_info.buf, 0))
			{
				// Trouble!
				return 1;
			}
			nandfmt_info.offset = 0;
			length -= amt;
			buf += amt;
			// printf ("offset=%d length=%d buf=%x\n", nandfmt_info.offset,
			//	length, (unsigned long)buf);
		}
	}
	return 0;
}

void nandfmt_show_erase_counts ()
{
	// Just scan
	printf ("size=%08x\n", (unsigned long)nandfmt_info.mtd.size);
	printf ("eb_cnt=%d\n", nandfmt_info.mtd.eb_cnt);
	printf ("eb_size=%d\n", nandfmt_info.mtd.eb_size);
	printf ("min_io_size=%d\n", nandfmt_info.mtd.min_io_size);
	printf ("subpage_size=%d\n", nandfmt_info.mtd.subpage_size);
	printf ("mean_ec=%d\n", (unsigned long) nandfmt_info.si->mean_ec);
	printf ("ok_cnt=%d\n", nandfmt_info.si->ok_cnt);
	printf ("empty_cnt=%d\n", nandfmt_info.si->empty_cnt);
	printf ("corrupted_cnt=%d\n", nandfmt_info.si->corrupted_cnt);
	printf ("alien_cnt=%d\n", nandfmt_info.si->alien_cnt);
	printf ("bad_cnt=%d\n", nandfmt_info.si->bad_cnt);
	printf ("good_cnt=%d\n", nandfmt_info.si->good_cnt);
	printf ("vid_hdr_offs=%d\n", nandfmt_info.si->vid_hdr_offs);
	printf ("data_offs=%d\n", nandfmt_info.si->data_offs);
	unsigned long i;
	int min=-1, max=-1, N=0, sum=0;
	for (i=nandfmt_info.current_i; i<nandfmt_info.end_i; i++)
	{
		if (!(i & 15) || i==nandfmt_info.current_i)
			printf ("\n%5x (%08x):", i, i*nandfmt_info.mtd.eb_size);
		printf (" %03x", nandfmt_info.si->ec[i] & 0xfff);
		if (nandfmt_info.si->ec[i] >= 0 && nandfmt_info.si->ec[i] <= EC_MAX)
		{
			sum += nandfmt_info.si->ec[i];
			N++;
			if (min == -1 || min > nandfmt_info.si->ec[i])
				min = nandfmt_info.si->ec[i];
			if (max == -1 || max < nandfmt_info.si->ec[i])
				max = nandfmt_info.si->ec[i];
		}
	}
	printf ("\nN=%d=0x%x", N, N);
	if (N)
	{
		printf (" Min=%d=0x%x", min, min);
		printf (" Max=%d=0x%x", max, max);
		printf (" Mean=%f=0x%x", (float)sum/N, (int)(sum/N+.5));
	}
	printf ("\n");
	nandfmt_free ();
}
