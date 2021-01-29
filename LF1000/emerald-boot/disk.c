/* disk.c - Initialize an SD card, scan the MBR for parition information
 *
 * We assume the old-style MBR without extended paritions.
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <mmc.h>
#include <disk.h>
#include <debug.h>

#define PARTITION_TABLE	0x01BE
#define MBR_SIGNATURE	0x01FE

#define PR_TYPE		0x04
#define PR_LBA		0x08
#define PR_SECTORS	0x0C

static u32 read4_unaligned(u8 *src)
{
	u32 res = 0;

	res += *(src + 0)<<0;
	res += *(src + 1)<<8;
	res += *(src + 2)<<16;
	res += *(src + 3)<<24;

	return res;
}

static u16 read2_unaligned(u8 *src)
{
	u16 res = 0;

	res += *(src + 0)<<0;
	res += *(src + 1)<<8;

	return res;
}

int disk_load_mbr(u32 sdhc_base, u32 *buf, struct disk_partition *partitions)
{
	int i;
	u8 *pr;

	db_puts("disk_load_mbr: sdhc_base=");db_int(sdhc_base); db_puts("\n");

	if (!buf || !partitions) {
		db_puts("disk_load_mbr -- return 1\n");
		return 1;
	}
	if (mmc_init(sdhc_base)) {
		db_puts("disk_load_mbr -- return 2\n");
		return 2;
	}
	if (mmc_read_sector(sdhc_base, 0, buf)) {
		db_puts("disk_load_mbr -- return 2\n");
		return 3;
	}
	
	/* make sure that the MBR magic signature matches */
	if (read2_unaligned((u8 *)buf + MBR_SIGNATURE) != 0xAA55) {
		db_puts("disk_load_mbr -- return 2\n");
		return 4;
	}

	for (i = 0; i < 4; i++) {
		pr = (u8 *)buf + PARTITION_TABLE + 16*i;

		partitions[i].type = *(pr + PR_TYPE);
		partitions[i].start = read4_unaligned(pr + PR_LBA);
		partitions[i].length = read4_unaligned(pr + PR_SECTORS);
	}

	db_puts("disk_load_mbr -- return 0\n");
	return 0;
}
