/* ramdisk.c -- ramdisk for nor boot usb
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef RAMDISK

#include <common.h>
#include <ramdisk.h>
#include <scsi.h>
#include <string.h>
#include <debug.h>
#include <usb_payload.h>

u32 GetRamDiskAddr();

#define pRamDisk	((u8 *)(GetRamDiskAddr()))


u8 * GetRamDiskPtr( u32 offset ) {	/* offset is in bytes */
	return &pRamDisk[offset];
}


static void InitMBR( u8 * pMBR) 
{
	int i;
	for (i = 0; i < BYTES_PER_SECTOR; ++i) pMBR[i] = 0;

	pMBR[0] = 0xEB;		// Jump instruction
	pMBR[1] = 0x3C;
	pMBR[2] = 0x90;
	
	strcpy( (char *)&pMBR[3], "MSDOS5.0");	// OEM Name

	pMBR[0x0B] = BYTES_PER_SECTOR % 256;	// bytes per sector (512)
	pMBR[0x0C] = BYTES_PER_SECTOR / 256;

	pMBR[0x0D] = SECTORS_PER_CLUSTER;		// sectors per cluster

	pMBR[0x0E] = SECTORS_PER_MBR;			// reserved sector count (MBR)
	//pMBR[0x0F] = 0;

	pMBR[0x10] = 2;		// number of FATs

	pMBR[0x11] = 16;	// max # of root dir entries (1 sector)
	//pMBR[0x12] = 0;

	pMBR[0x13] = SECTORS_PER_DISK % 256;	// total # of sectors
	pMBR[0x14] = SECTORS_PER_DISK / 256;

	pMBR[0x15] = 0xF8;						// media type (fixed disk)
	pMBR[0x16] = SECTORS_PER_FAT % 256;		// sectors per FAT
	pMBR[0x17] = SECTORS_PER_FAT / 256;
	
	pMBR[0x18] = 9;		// sectors per track
	//pMBR[0x17] = 0;
	
	pMBR[0x1A] = 2;		// # of heads
	//pMBR[0x1B] = 0;

	pMBR[0x1C] = 1;		// hidden sectors (reserved sector count?)
	//pMBR[0x1D] = 0;
	//pMBR[0x1E] = 0;
	//pMBR[0x1F] = 0;

	pMBR[0x26] = 0x29;	// extended boot signature
	
	pMBR[0x27] = 0x77;	// serial #
	pMBR[0x28] = 0x56;
	pMBR[0x29] = 0x34;
	pMBR[0x2A] = 0x12;

	strcpy( (char *)&pMBR[0x2B], "RAMDISK    ");
	strcpy( (char *)&pMBR[0x36], "FAT16   ");
	pMBR[0x1FE] = 0x55;	// boot sector signature
	pMBR[0x1FF] = 0xAA;
}

void InitFAT16( u8 * pFAT)
{
	int i;
	for (i = 0; i < SECTORS_PER_FAT * BYTES_PER_SECTOR; ++i) pFAT[i] = 0;
	pFAT[0] = 0xF8;
	pFAT[1] = 0xFF;
	pFAT[2] = 0xFF;
	pFAT[3] = 0xFF;
}

void InitRootDir( u8 * pRootDir )
{
	int i;
	for (i = 0; i < BYTES_PER_SECTOR; ++i) pRootDir[i] = 0;
}

static void FormatDisk(void) 
{
	InitMBR(&pRamDisk[0]);
	InitFAT16(&pRamDisk[SECTORS_PER_MBR * BYTES_PER_SECTOR]);	// first FAT
	InitFAT16(&pRamDisk[(SECTORS_PER_MBR + SECTORS_PER_FAT) 
						* BYTES_PER_SECTOR]);					// second FAT
	InitRootDir(&pRamDisk[(SECTORS_PER_MBR + 2 * SECTORS_PER_FAT) 
						  * BYTES_PER_SECTOR]);	// root directory
}

#define kDirEntryEmptyAndNoMore	0
#define kDirEntryDot			0x2e
#define kDirEntryErasedAndAvail	0xe5
#define kDirEntryErased0x05		0x05

#define kFileAttrReadOnly	0x01
#define kFileAttrHidden		0x02
#define kFileAttrSystem		0x04
#define kFileAttrVolable	0x08
#define kFileAttrSubDIr		0x10
#define kFileAttrArchive	0x20
#define kFileAttrDevice		0x40
#define kFileAttrUnused		0x80
#define kFileAttrLongName	0x0F

#define kTimeHourMask		0xf800
#define kTimeMinMask		0x07E0
#define kTimeSecMask		0x001F	// seconds/2

#define kDateYearMask		0xfe00	// year 0 is 1980
#define kDateMonthMask		0x01e0	// month 1 is January
#define kDateDayMask		0x001f	// day 1-31

typedef struct tagFatDirEntry {
	char fileName[8];
	char fileExt[3];
	u8   fileAttributes;
	u8	 reserved;
	u8	 createTime10msec;	// 0 - 199
	u16	 createTimeHMS;
	u16	 createDate;
	u16	 lastAccessDate;
	u16  EAIndex;
	u16	 lastModTime;
	u16  lastModDate;
	u16	 indexFirstCluster;
	u32	 fileLen;			// in bytes
} tFatDirEntry;

static inline u8 * GetRootDirectory() {
	
	return (pRamDisk +
			BYTES_PER_SECTOR * (SECTORS_PER_MBR + NUM_FATS * SECTORS_PER_FAT));
}

	// the name and extension arrays ought to be padded with spaces
static tFatDirEntry * FindRootDirEntry(char name[8], char ext[3]) {
	tFatDirEntry  * pDirEntry;
	int				i;

	// calculate the address of the root directory
	pDirEntry = (tFatDirEntry *)GetRootDirectory();

	// step through its entries.
	// if a non-empty entry has the specified name and extension,
	//	return its address
	// otherwise return NULL
	for (i = 0; i < NUM_ROOTDIR_ENTRIES; ++i, ++pDirEntry) {
		//db_puts("RootDir entry ");  db_putchar('0' + i); db_putchar('\n');
		if (pDirEntry->fileName[0] == kDirEntryEmptyAndNoMore) {
			i = NUM_ROOTDIR_ENTRIES;	// leave
			//db_puts("  all remaining entries are empty\n");
		}
		else if (   (pDirEntry->fileName[0] == kDirEntryErasedAndAvail)
				 || (pDirEntry->fileName[0] == kDirEntryErased0x05)) {
			//db_puts("  erased entry\n");
			continue;
		}
		else {
			int j;

			for (j = 0; j < sizeof(pDirEntry->fileName); ++j) {
				if (name[j] != pDirEntry->fileName[j]) {
					break;
				}
			}
			if (j >= 8 /*sizeof(name)*/) { // name matches; check the extension
				for (j = 0; j < sizeof(pDirEntry->fileExt); ++j) {
					if (ext[j] != pDirEntry->fileExt[j]) {
						break;
					}
				}
				if (j >= 3 /*sizeof(ext)*/) {	// both name and ext match
					break;
				}
			}
		}
	}
	if (i >= NUM_ROOTDIR_ENTRIES) {
		pDirEntry = NULL;
	}
	return pDirEntry;
}

static char nameKernel[8] = USB_PAYLOAD_NAME8;
static char extBin[3]     = USB_PAYLOAD_EXT3;

// returns 0 if found and copied ok.
// else returns error code
#define CK_ERROR_NO_FILE					1
#define CK_ERROR_BAD_FIRST_CLUSTER_INDEX	2
#define CK_ERROR_UNEXPECTED_END_CLUSTER		3
#define CK_ERROR_UNEXPECTED_END_LENGTH		4

#define min(a,b)	(((a) <= (b)) ? (a) : (b))

#include "include/cbf.h"
	// first we read data from a ramdisk cluster into this buffer.
	// Then we pass this buffer's contents to the cbf functions.
static u8 clusterBuffer[ SECTORS_PER_CLUSTER * BYTES_PER_SECTOR ];

int  CopyKernelBinFromRamdiskThruCbf( ) {
	tFatDirEntry * pKernelBinDirEntry;
	int		status;
	// Search the root directory for a file named USB_PAYLOAD
	pKernelBinDirEntry = FindRootDirEntry( nameKernel, extBin );
	if (NULL == pKernelBinDirEntry) {	// if not found
		status = CK_ERROR_NO_FILE;
        db_puts("CKBFR: not found\n");
	}
	else {
		u32		clusterIndex;
		u32 	sectorIndex;
		u32 	byteOffset;
		u32 	numLeftToCopy;
		u32		bytesToCopy;
		u8	  * pSrc;
		u8	  * pDest;
		u16   * pFAT;
		u32		i;

			// extract the file's length and the index of its first cluster
		clusterIndex  = pKernelBinDirEntry->indexFirstCluster;
		numLeftToCopy = pKernelBinDirEntry->fileLen;

			// NOTE: We use '2' and '0xFFEF' only because the file system 
            //       is FAT16.  If the type of file system is changed, the
            // test must also be changed.

		if (   (clusterIndex < 2) || (clusterIndex > 0xFFEF)) {
			status = CK_ERROR_BAD_FIRST_CLUSTER_INDEX;
            db_puts("CKBFR: bad first cluster\n");
		}
		else {
			int cbf_status = 1;

			cbf_init((void *)calc_SDRAM_ADDRESS());
			//	Calculate the address of the first FAT
			pFAT  = (u16 *)(pRamDisk + SECTORS_PER_MBR * BYTES_PER_SECTOR);

			pDest = clusterBuffer;

			while (cbf_status == 1) 
			{
				//   From the cluster index, calculate the offset (in sectors 
				//		and then in bytes) from the beginning of the ramdisk.
				sectorIndex = SECTORS_PER_MBR
								+ SECTORS_PER_ROOTDIR 
								+ (SECTORS_PER_FAT * NUM_FATS)
								+ (clusterIndex - 2) * SECTORS_PER_CLUSTER;
				byteOffset  = sectorIndex * BYTES_PER_SECTOR;

					//	Set pSrc to the address of the cluster's first byte.
				pSrc  = pRamDisk + byteOffset;
	
					//	Copy min(numLeftToCopy, clusterSize) bytes from the 
					//	cluster to the destination.
				bytesToCopy = min(numLeftToCopy, 
								  (SECTORS_PER_CLUSTER * BYTES_PER_SECTOR));
				for (i = 0; i < bytesToCopy; ++i) {
					pDest[i] = *pSrc++;
				}

				// To speed reading from ramdisk, pass &pDest as the 3rd arg;
				// this eliminates the need for an extra copy.  The kernel image
				// is copied from the ramdisk to the kernel destination buffer.
				cbf_status = cbf_process_chunk( pDest, bytesToCopy, &pDest);

				if (cbf_status != 1)	// Error or Done; prepare to leave
				{
					status = cbf_status;
				}
				else
				{
						//	decrement numLeftToCopy
					numLeftToCopy -= bytesToCopy;

						//	from the cluster's entry in the FAT, find the index 
						//	of the file's next cluster.
					clusterIndex = *(pFAT + clusterIndex);

						//	if numLeftToCopy is <= 0  
						//		if next cluster index indicates we just
                        //      processed the last cluster, break and return
                        //      ReadOK indication; else return error indication.
					if (numLeftToCopy <= 0) {
						if (clusterIndex >= 0xFFF8) {
							status = 0;
                            db_puts("CKBFR: copied ok\n");
						}
						else {
							status = CK_ERROR_UNEXPECTED_END_CLUSTER;
                            db_puts("CKBFR: unExEndCluster\n");
						}
						break;
					}
						//	else if next cluster index indicates we just
                        //  processed the file's last cluster, return error
                        //  indication.
					else if (clusterIndex >= 0xFFF8) {
						status = CK_ERROR_UNEXPECTED_END_LENGTH;
                        db_puts("CKBFR: unExEndLen\n");
						break;
					}
					//	else loop.
				}	// cbf_status == 1
			}
		}
	}
	return status;
}


void DiskInit( void )
{ 
	// pRamDisk = (u8 *)GetRamDiskAddr();
	FormatDisk();
} 

#endif	// ifdef RAMDISK
