/* ramdisk.h    -- ram-resident FAT file system
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef RAMDISK_H
#define RAMDISK_H

#define RAMDISK_SECTOR 		0x200
#define RAMDISK_SECTOR_SIZE 0x200
#define BYTES_PER_SECTOR 	0x200

// These definitions result in a 12MB FAT16 ramdisk
// (The number of clusters dictates whether FAT12 or FAT16 is used.  FAT12
//  supports no more than 4077 clusters.)
//#define SECTORS_PER_DISK	32768 /* New for 16MB; here's the 12MB value: 24576*/
#define SECTORS_PER_DISK	24576
#define RAMDISK_SIZE   		(RAMDISK_SECTOR_SIZE * SECTORS_PER_DISK)
#define SECTORS_PER_CLUSTER	4
//#define SECTORS_PER_FAT		34 /* New for 16MB; here's the 12MB value: 25 */
#define SECTORS_PER_FAT		25
#define NUM_FATS			2
#define NUM_DATA_SECTORS	(SECTORS_PER_DISK - 1 - 2 * SECTORS_PER_FAT - 1)
							// total - 1 for MBR - FATs - rootDir
							// 24524
#define NUM_DATA_CLUSTERS	(NUM_DATA_SECTORS / SECTORS_PER_CLUSTER)
							// 6131
#define BYTES_PER_FAT		(4 + (2 * NUM_DATA_CLUSTERS))
							// 12266
							// @ 512/sector ==> 25  6 sectors
#define SECTORS_PER_MBR		1
// We allow only as many root directory entries as fit in one sector
//	(512 / 32) == 16
#define SECTORS_PER_ROOTDIR	1
#define NUM_ROOTDIR_ENTRIES	16


#endif /* RAMDISK_H */


