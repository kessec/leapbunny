/* dpc.h  -- prototype of the display controller driver initialization function
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_DPC_H
#define LIGHTNING_BOOT_DPC_H

void dpc_enable(void);
void dpc_init(u8 thb, u8 thf, u8 tvb, u8 tvf);

#endif /* LIGHTNING_BOOT_DPC_H */
