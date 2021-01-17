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

// Low level access via ioctls

//////////////////////////////////////////////////////////////////////////////

void usage (const char *v)
{
	printf ("%s Version 1.1\n", v);
	printf ("Usage: %s [-h] [-v] [-d] [-s offset] [-l length] /dev/mtdX\n", v);
	printf ("  -h: this help\n");
	printf ("  -v: turn on verbose mode\n");
	printf ("  -d: just list erase counts instead of flashing\n");
	printf ("  -s: set starting offset of partition in bytes\n");
	printf ("  -l: set length of partition in bytes\n");
	printf ("  -u: flash a UBI image (preserve erase counts\n");
	printf ("Flashes with ubi image data from stdin, bumping erase counters\n");
	printf ("Automatically pads out short images to full partition (or up to length)\n");
	exit (1);
}

main (int argc, char **argv)
{
	int verbose = 0, scan = 0, in_ubi_mode = 0;
	unsigned long start=0, length=0;
	while (1)
	{
		int option_index = 0;
		static const char *short_options = "hds:l:uv";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"dump", no_argument, 0, 'd'},
			{"start", required_argument, 0, 's'},
			{"length", required_argument, 0, 'l'},
			{"ubi", no_argument, 0, 'u'},
			{"verbose", no_argument, 0, 'v'},
			{0, 0, 0, 0},
		};

		int cc = getopt_long(argc, argv, short_options,
				long_options, &option_index);
		if (cc == EOF)
			break;
		switch (cc) 
		{
		case 'h':
			usage (argv[0]); break;
		case 'v':
			verbose = 1; break;
		case 'd':
			scan = 1; break;
		case 's':
			start = strtol(optarg, NULL, 0); break;
		case 'l':
			length = strtol(optarg, NULL, 0); break;
		case 'u':
			in_ubi_mode = 1; break;
		default:
			usage (argv[0]); break;
		}
	}
	if ((argc - optind) != 1)
		usage (argv[0]);
	if (scan)
	{
		if (nandfmt_open (argv[optind], start, length, 0, 0))
			exit (1);
		nandfmt_show_erase_counts ();
		return 0;
	}

	// Format
#define SIZE 1024
	char *buf = malloc (SIZE);
	if (!buf)
	{
		printf ("Can't malloc %d\n", SIZE);
		exit (2);
	}
	if (nandfmt_open (argv[optind], start, length, in_ubi_mode, verbose))
		exit (1);
	int any = 0;
	while (1)
	{
		int amt = read (fileno (stdin), buf, SIZE);
		if (amt < 0)
		{
			printf ("Trouble reading\n");
			exit (1);
		}
		if (amt == 0)
			break;	// done
		any += amt;
		if (nandfmt_write (buf, amt))
		{
			printf ("Trouble formatting!\n");
			exit (1);
		}
	}
	free (buf);
	if (any)
		nandfmt_close (NULL);
	else
	{
		printf ("No input found; aborting %s\n", argv[0]);
		exit (1);
	}
	return 0;
}
