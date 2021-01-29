/* rle.h  -- decode run-length-encoded images
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RLE_H__
#define __RLE_H__

int fill_fb (const u32 value, u32 *outbuf);
int rle_decode (const u32 *inbuf, u32 *outbuf);

#endif /* __RLE_H__ */
