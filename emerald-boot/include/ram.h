/* ram.h  -- infomation about the platform's ram
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_RAM_H
#define LIGHTNING_BOOT_RAM_H

extern char *probe_ram ();
extern u32 get_frame_buffer_addr ();
extern u32 get_frame_buffer_size ();
#endif
