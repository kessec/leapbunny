/* mmc.h - Basic polling MMC/SD driver
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MMC_H__
#define __MMC_H__


/* Enable the host controller and attempt to find and initialize an SD card.
 * The card must be plugged in prior to calling this function.  Returns 0 if
 * the controller was enabled and a card was found and initialized, otherwise a
 * non-zero error code is returned. 
 *
 * TODO: we should querry the card's supported block size and provide it from
 *       this function.  For now we assume 512 bytes and error out if that
 *       isn't allowed by the card. */
int mmc_init(u32 sdhc_base);
void mmc_exit(u32 sdhc_base);

/* Read sector number 'sector' into destination buffer 'dest' which must be
 * large enough to hold a sector's worth of information.  Call mmc_init() once
 * before using this function as it depends on the card being detected and
 * querried by mmc_init().
 *
 * This function returns 0 on success (a sector's worth of data was read into
 * 'dest').  Otherwise it returns a 1 if the card wasn't querried yet or if
 * 'dest' is NULL.  It returns other non-zero error codes if an error occurs
 * while reading from the card, these can be masked to see where the failure
 * occured. */
int mmc_read_sector(u32 sdhc_base, u32 sector, u32 *dest);
int mmc_read_multiblock(u32 sdhc_base, u32 sector, u32 num_sectors, u32 *dest);

#endif /* __MMC_H__ */
