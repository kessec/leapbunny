/* cbf.h -- access to Common Boot Format, a simple packing format for binaries.
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CBF__H__
#define __CBF__H__

void cbf_init ();
int cbf_process_chunk (u8 *buf, u32 len, u8 **suggested_buf);
int cbf_get_jump_address (void **jump);
int cbf_get_status ();

#endif /* __CBF__H__ */
