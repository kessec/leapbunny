#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <asm/types.h>
#include <mtd/mtd-user.h>

int main (int c, char **v)
{
	static unsigned char oobbuf[128];
	struct mtd_oob_buf oob = {0, 16, oobbuf};
	unsigned long start_addr, end_addr, ofs, bs;
	mtd_info_t meminfo;
	int fd, omit_ff=0;
	if (c<2)
	{
		printf ("Usage: oobdump </dev/mtdX> [-x]\n-x omit ff's\n");
		exit (1);
	}
	if (c>2)
	{
		omit_ff = 1;
	}
	if ((fd = open(v[1], O_RDONLY)) == -1)
	{
		perror(v[1]);
		exit (2);
	}
	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit (3);
	}
	start_addr = 0;
	end_addr = meminfo.size;

	/* Read the real oob length */
	oob.length = meminfo.oobsize;
	bs = meminfo.writesize;

	printf ("writesize=%d oobsize=%d size=%08x\n",
		meminfo.writesize, meminfo.oobsize, meminfo.size);
	for (ofs = start_addr; ofs < end_addr ; ofs+=bs) {
		unsigned long i;
		int any = 1;
		if ((ofs & 0xffffff) == 0)
		{
			fprintf (stderr, "%08x...\n", ofs);
		}
		oob.start = ofs;
		if (ioctl(fd, MEMREADOOB, &oob) != 0) {
			perror("ioctl(MEMREADOOB)");
			exit (4);
		}
		if (omit_ff)
		{
			any = 0;
			for (i = 0; i < meminfo.oobsize; i ++) {
				if (oobbuf[i] != 0xff)
					any = 1;
			}
		}
		if (any)
		{
			for (i = 0; i < meminfo.oobsize; i += 16) {
				printf ("  OOB Data %08x: %02x %02x %02x %02x %02x %02x "
					"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					ofs+i,
					oobbuf[i], oobbuf[i+1], oobbuf[i+2],
					oobbuf[i+3], oobbuf[i+4], oobbuf[i+5],
					oobbuf[i+6], oobbuf[i+7], oobbuf[i+8],
					oobbuf[i+9], oobbuf[i+10], oobbuf[i+11],
					oobbuf[i+12], oobbuf[i+13], oobbuf[i+14],
					oobbuf[i+15]);
			}
		}
	}
	return 0;
}
