/* rle.c -- Cheepo Run Length Encoded decoder for images
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <board.h>
#include <rle.h>

#if BPP!=4
#error	"This code is built for 4 BPP"
#endif


/* Returns 0.
 * Always stores FB_VIRTUAL_XRES * FB_VIRTUAL_YRES 32-bit pixels, starting at
 * outbuf, so outbuf ought to point to a buffer that's at least that big. */
int rle_decode (const u32 *inbuf, u32 *outbuf)
{
	u32 *in = (u32 *)inbuf;
	register u32 *out = (u32 *)outbuf;
	int totallen = FB_VIRTUAL_XRES * FB_VIRTUAL_YRES;
	int declen = totallen;
#if FB_VIRTUAL_XRES==320
	int xstride = 1;
	int ystride = 0;
#else
	int xstride = FB_VIRTUAL_XRES;
	int ystride = -1;
	out += xstride-1;
#endif

	while (declen > 0) {
		u32 rgba = *in++;
		u8 n = rgba>>24;
		u8 i;
		
		rgba |= 0xff << 24;

		/* don't let bad data cause us to write beyond the end */
		if (n > declen)
			n = declen; /* of the destination buffer. */
		for (i = 0; i < n; i++)
		{
			*out = rgba;
			out += xstride;
			if (out-outbuf >= totallen) // Move to top of next line
			{
				out-=totallen;
				out+=ystride; // one pixel over
			}
		}
		declen -= n;
	}

	return declen;
}

int fill_fb (const u32 value, u32 *outbuf)
{
	register u32 *out = (u32 *)outbuf;
	int declen = FB_VIRTUAL_XRES * FB_VIRTUAL_YRES;

	for ( ;declen > 16; declen -= 16) {
		*out++ = value;
		*out++ = value;
		*out++ = value;
		*out++ = value;

		*out++ = value;
		*out++ = value;
		*out++ = value;
		*out++ = value;

		*out++ = value;
		*out++ = value;
		*out++ = value;
		*out++ = value;

		*out++ = value;
		*out++ = value;
		*out++ = value;
		*out++ = value;
	}
	for ( ;declen > 0; --declen) {
		*out++ = value;
	}
	return 0;
}



