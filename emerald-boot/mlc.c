/* mlc.c -- Basic Multi-Layer Controller (MLC) Driver for displaying boot
 *          splash screen.
 *
 * Copyright 2007-2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "include/autoconf.h"   /* for partition info */
#include "include/mach-types.h" /* for machine info */
#include <mach/platform.h>
#include <mach/common.h>

#include "include/board.h"
#include "include/mlc_hal.h"
#include "include/ram.h"

#define MLC32(x)	REG32(LF1000_MLC_BASE+x)

#define MLC_LOCK_SIZE	8
#define MLC_PRIORITY	0

/*
 * Basic MLC initialization: our goal is just to bring up Layer 0 so that it 
 * can be used to display an RGB splash screen.
 */
void mlc_init(void)
{
	/* don't do anything if the MLC was already enabled */
	if (MLC32(MLCCONTROLT) & (1<<MLCENB))
		return;

	/* dynamic bus clock, PCLK on CPU access */
	MLC32(MLCCLKENB) &= ~(0xF);
	MLC32(MLCCLKENB) |= (2<<BCLKMODE); 

	/* set resolution */
	MLC32(MLCSCREENSIZE) = ( ((X_RESOLUTION-1)<<SCREENWIDTH)
                            |((Y_RESOLUTION-1)<<SCREENHEIGHT));

	/* set up layer priority, turn off field */	
	MLC32(MLCCONTROLT) &= ~((0x3<<PRIORITY)|(1<<FIELDENB));
	MLC32(MLCCONTROLT) |= (MLC_PRIORITY<<PRIORITY);

	/* make the background white */
	MLC32(MLCBGCOLOR) = 0xFFFFFF;

	/* 
	 * Set up Layer 0 
	 */

	MLC32(MLCADDRESS0)   = get_frame_buffer_addr();	
	MLC32(MLCCONTROL0)  &= ~(3<<LOCKSIZE);
	MLC32(MLCCONTROL0)  |= ((MLC_LOCK_SIZE/8)<<LOCKSIZE);
	MLC32(MLCHSTRIDE0)   = BPP; /* 3 bytes per pixel */
	MLC32(MLCVSTRIDE0)   = BPP*X_RESOLUTION;
	MLC32(MLCLEFTRIGHT0) = ((0<<LEFT)|((X_RESOLUTION-1)<<RIGHT));
	MLC32(MLCTOPBOTTOM0) = ((0<<TOP)|((Y_RESOLUTION-1)<<BOTTOM));
	MLC32(MLCCONTROL0)  &= ~(0xFFFF<<FORMAT);
	MLC32(MLCCONTROL0)  |= (0xC653<<FORMAT); /* B8G8R8 */
	MLC32(MLCCONTROL0)  |= (1<<DIRTYFLAG);

	/* 
	 * Turn on Layer 0 
	 */

	BIT_SET(MLC32(MLCCONTROL0), PALETTEPWD); /* power up */
	BIT_CLR(MLC32(MLCCONTROL0), PALETTESLD); /* unsleep */
	BIT_SET(MLC32(MLCCONTROL0), LAYERENB);	 /* enable */
	BIT_SET(MLC32(MLCCONTROL0), DIRTYFLAG);	 /* apply */

	/*
	 * Finally, turn on the MLC!
	 */

	BIT_SET(MLC32(MLCCONTROLT), PIXELBUFFER_PWD); /* power up */
	BIT_SET(MLC32(MLCCONTROLT), PIXELBUFFER_SLD); /* unsleep */
	BIT_SET(MLC32(MLCCONTROLT), MLCENB);          /* enable */
	BIT_SET(MLC32(MLCCONTROLT), DITTYFLAG);       /* apply */
}
