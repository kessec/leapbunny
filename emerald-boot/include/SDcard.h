/* SDcard.h
 *
 * Copyright 2010 Joe Burks
 *
 * Contact: Joe Burks (joe@burks-family.us)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SDCARD_H
#define SDCARD_H

extern u8 SdReadBlock(u32 Block, u8 *buff);
extern u8 SdWriteBlock(u32 Block, u8 *buff);
extern u8 SdCardInit(void);
extern void clkDelay(void);

#endif //SDCARD_H
