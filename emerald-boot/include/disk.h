/* disk.h - MBR and basic disk support
 *
 * We assume the old-style MBR without extended paritions.
 *
 * TODO: we can support variable sector sizes, as informed by the MMC driver
 *       which in turn can tell us what the card supports.  In that case we
 *       shouldn't use DISK_SECTOR_SIZE but instead we can have disk_load_mbr()
 *       copy the sector size to a location pointed to by an argument.  For now
 *       we assume 512 byte sectors which is common on SD cards.
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DISK_H__
#define __DISK_H__

#define DISK_TYPE_LINUX		0x83
#define DISK_SECTOR_SIZE	512
#define DISK_SECTOR_SHIFT	9

struct disk_partition {
	unsigned char type;	/* parititon type, ex: DISK_TYPE_LINUX */
	unsigned int start;	/* LBA, ie: sector number on the disk */
	unsigned int length;	/* number of sectors in this paritition */
};

/* Try to detect a disk (SD card) by using mmc_init() and reading the 0th
 * sector on the card.  Parse that sector (the MBR) and try to fill in the
 * paritions structure with information on the four partitions on the disk.
 * 
 * The buffer pointer 'buf' must point to a buffer of at least DISK_SECTOR_SIZE
 * bytes and that will be used to read and parse the MBR.
 *
 * The 'parititions' pointer must point to an array of 4 items of type
 * 'struct disk_partition'.
 *
 * This function returns 0 on success (we have a card, it has an MBR, and we
 * read information about the partitions).  It returns 1 if either arguement is
 * NULL, 2 if there is no SD card, 3 if reading the 0th sector failed, and 4 if
 * the sector does not seem to contain a valid MBR. */
int disk_load_mbr(u32 sdhc_base, u32 *buf, struct disk_partition *partitions);

#endif /* __DISK_H__ */
