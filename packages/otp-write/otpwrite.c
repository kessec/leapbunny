/*
 *  otpwrite.c
 *
 *  Copyright (C) 2008 Yifeng Tu (ytu@leapfrog.com)
 *  Based on MTD utility nandwrite.c 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Overview:
 *   This utility writes a binary image directly to a SANDISK OTP CHIP
 */

#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>

#include <asm/types.h>
#include "mtd/mtd-user.h"

#define PROGRAM "otpwrite"
#define VERSION "$Revision: 1.00 $"

#define MAX_PAGE_SIZE	4096
#define MAX_OOB_SIZE	128

/*
 * Buffer array used for writing data
 */
static unsigned char writebuf[MAX_PAGE_SIZE];
static unsigned char readbuf[MAX_PAGE_SIZE];
static unsigned char oobbuf[MAX_OOB_SIZE];
static unsigned char oobreadbuf[MAX_OOB_SIZE];

static void display_help (void)
{
	printf(
"Usage: otpwrite [OPTION] MTD_DEVICE [INPUTFILE|-]\n"
"Writes to the specified MTD device corresponding to SANDISK OTP.\n"
"\n"
"  -c x, --counter=x    Maximum retry for programming OTP (default is 4)\n"	
"  -n, --noecc             Write without ecc\n"
"  -o, --oob               Image contains oob data\n"
"  -p, --pad               Pad to page size\n"
"  -q, --quiet             Don't display progress messages\n"
"      --help              Display this help and exit\n"
"      --version           Output version information and exit\n"
	);
	exit (EXIT_SUCCESS);
}

static void display_version (void)
{
	printf(PROGRAM " " VERSION "\n");
	exit (EXIT_SUCCESS);
}

static const char	*standard_input = "-";
static const char	*mtd_device, *img;
static int		mtdoffset = 0;
static bool		quiet = false;
static bool		writeoob = false;
static bool		noecc = true;
static bool		pad = false;
static int		max_retry = 3;

static void process_options (int argc, char * const argv[])
{
	int error = 0;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "nopqc:";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"noecc", no_argument, 0, 'n'},
			{"oob", no_argument, 0, 'o'},
			{"pad", no_argument, 0, 'p'},
			{"quiet", no_argument, 0, 'q'},
			{"count", required_argument, 0, 'c'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
			case 0:
				switch (option_index) {
					case 0:
						display_help();
						break;
					case 1:
						display_version();
						break;
				}
				break;
			case 'q':
				quiet = true;
				break;
			case 'n':
				noecc = true;
				break;
			case 'o':
				writeoob = true;
				break;
			case 'p':
				pad = true;
				break;
			case 'c':
				max_retry = strtol(optarg, NULL, 0);
				break;			
			case '?':
				error++;
				break;
		}
	}

	argc -= optind;
	argv += optind;

	/*
	 * There must be at least the MTD device node positional
	 * argument remaining and, optionally, the input file.
	 */

	if (argc < 1 || argc > 2 || error)
		display_help ();

	mtd_device = argv[0];

	/*
	 * Standard input may be specified either explictly as "-" or
	 * implicity by simply omitting the second of the two
	 * positional arguments.
	 */

	img = ((argc == 2) ? argv[1] : standard_input);
}

static void erase_buffer(void *buffer, size_t size)
{
	const uint8_t kEraseByte = 0xff;

	if (buffer != NULL && size > 0) {
		memset(buffer, kEraseByte, size);
	}
}

void print_mtd_info(struct mtd_info_user *pmeminfo)
{
	printf("meminfo.writesize = %d\n", pmeminfo->writesize);
}


/*
 * Main program
 */
int main(int argc, char * const argv[])
{
	int cnt = 0;
	int fd = -1;
	int ifd = -1;
	int imglen = 0, pagelen;
	struct mtd_info_user meminfo;
	struct mtd_oob_buf oob;
	int readlen;
	int oobinfochanged = 0;
	struct nand_oobinfo old_oobinfo;
	int i,retry;

	process_options(argc, argv);

	erase_buffer(oobbuf, sizeof(oobbuf));

	if (pad && writeoob) {
		fprintf(stderr, "Can't pad when oob data is present.\n");
		exit (EXIT_FAILURE);
	}

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) < 0) {
		perror(mtd_device);
		exit (EXIT_FAILURE);
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit (EXIT_FAILURE);
	}
	
	// print_mtd_info(&meminfo);
	
	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 16 && meminfo.writesize == 512) &&
			!(meminfo.oobsize == 8 && meminfo.writesize == 256) &&
			!(meminfo.oobsize == 64 && meminfo.writesize == 2048) &&
			!(meminfo.oobsize == 128 && meminfo.writesize == 4096)) {
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(fd);
		exit (EXIT_FAILURE);
	}

	oob.length = meminfo.oobsize;
	oob.ptr = noecc ? oobreadbuf : oobbuf;

	ifd = open(img, O_RDONLY);
	if (ifd == -1) {
		perror(img);
		goto restoreoob;
	}

	pagelen = meminfo.writesize + ((writeoob) ? meminfo.oobsize : 0);

	for(retry=0; retry<max_retry; retry++) {

		mtdoffset = 0;
		
		imglen = lseek(ifd, 0, SEEK_END);
		lseek (ifd, 0, SEEK_SET);

		// Check, if file is page-aligned
		if ((!pad) && ((imglen % pagelen) != 0)) {
			fprintf (stderr, "Input file is not page-aligned. Use the padding "
					 "option.\n");
			goto closeall;
		}

		// Check, if length fits into device
		if ( ((imglen / pagelen) * meminfo.writesize) > (meminfo.size - mtdoffset)) {
			fprintf (stderr, "Image %d bytes, NAND page %d bytes, OOB area %u bytes, device size %u bytes\n",
					imglen, pagelen, meminfo.writesize, meminfo.size);
			perror ("Input file does not fit into device");
			goto closeall;
		}

		if (imglen != 67108864) {
			fprintf (stderr, "Image size %d bytes, should be 67108864\n", imglen);
			perror ("Input file size wrong");
			goto closeall;
		}

		/*
		 * Get data from input and write to the device while there is
		 * still input to read and we are still within the device
		 * bounds. Note that in the case of standard input, the input
		 * length is simply a quasi-boolean flag whose values are page
		 * length or zero.
		 */
		if (ioctl (fd, SDOTPPREP) != 0) {
			perror ("SDOTPPREP");
			close (fd);
			exit (EXIT_FAILURE);
		}

		
		while (imglen && (mtdoffset < meminfo.size)) {
			// new eraseblock , check for bad block(s)
			int tinycnt = 0;

			readlen = meminfo.writesize;

			if (pad && (imglen < readlen))
			{
				readlen = imglen;
				erase_buffer(writebuf + readlen, meminfo.writesize - readlen);
			}

			/* Read Page Data from input file */
			while(tinycnt < readlen) {
				cnt = read(ifd, writebuf + tinycnt, readlen - tinycnt);
				if (cnt == 0) { // EOF
					break;
				} else if (cnt < 0) {
					perror ("File I/O error on input file");
					goto closeall;
				}
				tinycnt += cnt;
			}

			if (writeoob) {
				int tinycnt = 0;

				while(tinycnt < readlen) {
					cnt = read(ifd, oobreadbuf + tinycnt, meminfo.oobsize - tinycnt);
					if (cnt == 0) { // EOF
						break;
					} else if (cnt < 0) {
						perror ("File I/O error on input file");
						goto closeall;
					}
					tinycnt += cnt;
				}

				if (!noecc) {
					int i, start, len;
					/*
					 *  We use autoplacement and have the oobinfo with the autoplacement
					 * information from the kernel available
					 *
					 * Modified to support out of order oobfree segments,
					 * such as the layout used by diskonchip.c
					 */
					if (!oobinfochanged && (old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE)) {
						for (i = 0;old_oobinfo.oobfree[i][1]; i++) {
							/* Set the reserved bytes to 0xff */
							start = old_oobinfo.oobfree[i][0];
							len = old_oobinfo.oobfree[i][1];
							memcpy(oobbuf + start,
									oobreadbuf + start,
									len);
						}
					} else {
						/* Set at least the ecc byte positions to 0xff */
						start = old_oobinfo.eccbytes;
						len = meminfo.oobsize - start;
						memcpy(oobbuf + start,
								oobreadbuf + start,
								len);
					}
				}
				/* Write OOB data first, as ecc will be placed in there*/
				oob.start = mtdoffset;
				if (ioctl(fd, MEMWRITEOOB, &oob) != 0) {
					perror ("ioctl(MEMWRITEOOB)");
					goto closeall;
				}
				imglen -= meminfo.oobsize;
			}

			/* Write out the Page data */
			if (pwrite(fd, writebuf, meminfo.writesize, mtdoffset) != meminfo.writesize) {
				perror ("pwrite");
				printf("otp write error !!!\n");
				goto closeall;
			}
			imglen -= readlen;
			mtdoffset += meminfo.writesize;
			
			if((mtdoffset & 0x0FFFFF) == 0) {
				printf("written to offset 0x%x\n", mtdoffset);
			}
		}

		// first unaligned page is cached in a very special way, need to flush.
		pread(fd, readbuf, 64, 1024+8);

		printf("OTP write finished, verify now, please wait ...\n");
		mtdoffset = 0;
		imglen = lseek(ifd, 0, SEEK_END);
		lseek (ifd, 0, SEEK_SET);

		while (imglen && (mtdoffset < meminfo.size)) {
			// new eraseblock , check for bad block(s)

			int tinycnt = 0;
			readlen = meminfo.writesize;

			/* Read Page Data from input file */
			while(tinycnt < readlen) {
				cnt = read(ifd, writebuf + tinycnt, readlen - tinycnt);
				if (cnt == 0) { // EOF
					break;
				} else if (cnt < 0) {
					perror ("File I/O error on input file");
					goto closeall;
				}
				tinycnt += cnt;
			}
			
			/* Read the Page data */
			if (pread(fd, readbuf, meminfo.writesize, mtdoffset) != meminfo.writesize) {
				perror ("pread");
				printf("otp read error !!!\n");
				goto closeall;
			}
			imglen -= readlen;
			
			/* compare the results */
			for(i=0; i<meminfo.writesize; i++) {
				if(writebuf[i] != readbuf[i]) {
					printf("verify OTP programming failed, retry %d \n", retry);
					break;
				}
			}
			if(i!=meminfo.writesize)
				break;
			
			mtdoffset += meminfo.writesize;
				
		}
		
		if(imglen == 0) {
			printf("verify success !\n");
			break;
		}
	}
	
closeall:
	close(ifd);

restoreoob:
	if (oobinfochanged == 1) {
		if (ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0) {
			perror ("MEMSETOOBSEL");
			close (fd);
			exit (EXIT_FAILURE);
		}
	}

	close(fd);

	if ((ifd != STDIN_FILENO) && (imglen > 0)) {
		perror ("OTP programming failed\n");
		exit (EXIT_FAILURE);
	}

	/* Return happy */
	return EXIT_SUCCESS;
}
