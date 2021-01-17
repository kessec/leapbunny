/* rle.c -- Cheepo Run Length Encoded decoder for images
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <mach/common.h>
#include "include/autoconf.h"
#include "include/board.h"
#include "include/debug.h"
#include "include/rle.h"

#if BPP!=4
#error	"This code is built for 4 BPP"
#endif

// Returns 0.
// Always stores X_RESOLUTION * Y_RESOLUTION 32-bit pixels, starting at outbuf,
// so outbuf ought to point to a buffer that's at least that big.
int rle_decode (const u32 *inbuf, u32 *outbuf)
{
	u32 *in = (u32 *)inbuf;
	register u32 *out = (u32 *)outbuf;
	// Exit on exhausted output
	int declen = X_RESOLUTION * Y_RESOLUTION;
	while (declen>0)
	{
		u32 rgba = *in++;
		u8 n = rgba>>24, i;
		rgba |= 0xff << 24;
        if (n > declen) // don't let bad data cause us to write beyond the end
            n = declen; // of the destination buffer.
		for (i=0; i<n; i++)
			*out++ = rgba;
		declen -= n;
	}
	return declen;
}
