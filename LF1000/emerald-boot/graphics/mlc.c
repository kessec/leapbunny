/* mlc.c -- Basic Multi-Layer Controller (MLC) Driver for displaying boot
 *          splash screen.
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <board.h>
#include <base.h>
#include <ram.h>

#define MLC32(x)	REG32(MLC_BASE+x)

/* settings */
#define MLC_LOCK_SIZE	8
#define MLC_FORMAT	0xC653	/* BGR888 */

/* registers as offsets from MLC_BASE */

/* MLC 'top' registers */
#define MLCCONTROLT	0x00
#define MLCSCREENSIZE	0x04
#define MLCBGCOLOR	0x08
#define MLCCLKENB	0x3C0

/* layer 0 registers  */
#define MLCLEFTRIGHT0	0x0C
#define MLCTOPBOTTOM0	0x10
#define MLCCONTROL0	0x24
#define MLCHSTRIDE0	0x28
#define MLCVSTRIDE0	0x2C
#define MLCADDRESS0	0x38

/* MLC TOP CONTROL REGISTER (MLCCONTROLT) */
#define PIXELBUFFER_PWD		11	/* pixel buffer power on/off */
#define PIXELBUFFER_SLD		10	/* pixel buffer sleep mode */
#define PRIORITY		8	/* layer order */
#define DITTYFLAG		3	/* apply changes */
#define MLCENB			1
#define FIELDENB		0

/* MLC SCREEN SIZE REGISTER (MLCSCREENSIZE) */
#define SCREENHEIGHT		16
#define SCREENWIDTH		0

/* MLC RGB Layer n Control Register (MLCCONTROLn) */
#define FORMAT			16	/* see table 21-5 */
#define PALETTEPWD		15	/* layer n palette table on/off */
#define PALETTESLD		14	/* layer n palette table sleep mode */
#define LOCKSIZE		12	/* memory read size */
#define LAYERENB		5	/* enable the layer */
#define DIRTYFLAG		4	/* apply changes */

/* MLC RGB Layer n Left Right Register (MLCLEFTRIGHTn) */
#define LEFT			16
#define RIGHT			0

/* MLC RGB Layer n Top Bottom Register (MLCTOPBOTTOMn) */
#define TOP			16
#define BOTTOM			0

/* MLC CLOCK GENERATION ENABLE REGISTER (MLCCLKENB) */
#define BCLKMODE		0

/*
 * Basic MLC initialization: our goal is just to bring up Layer 0 so that it 
 * can be used to display an RGB splash screen.
 */
void mlc_init(u16 xres, u16 yres)
{
	u16 xoff = xres > FB_VIRTUAL_XRES ? (xres - FB_VIRTUAL_XRES)/2 : 0;
	u16 yoff = yres > FB_VIRTUAL_YRES ? (yres - FB_VIRTUAL_YRES)/2 : 0;

	/* don't do anything if the MLC was already enabled */
	if (MLC32(MLCCONTROLT) & (1<<MLCENB))
		return;

	/* dynamic bus clock, PCLK on CPU access */
	MLC32(MLCCLKENB) &= ~(0xF);
	MLC32(MLCCLKENB) |= (2<<BCLKMODE); 

	/* set resolution */
	MLC32(MLCSCREENSIZE) = (((xres-1)<<SCREENWIDTH)|
			((yres-1)<<SCREENHEIGHT));

	/* set layer priority = 0 (layer 0 on top), turn off field */	
	MLC32(MLCCONTROLT) &= ~((0x3<<PRIORITY)|(1<<FIELDENB));

	/* make the background white */
	MLC32(MLCBGCOLOR) = 0xFFFFFF;

	/* set up Layer 0 */

	MLC32(MLCADDRESS0) = get_frame_buffer_addr();	
	MLC32(MLCCONTROL0) &= ~(3<<LOCKSIZE);
	MLC32(MLCCONTROL0) |= ((MLC_LOCK_SIZE/8)<<LOCKSIZE);
	MLC32(MLCHSTRIDE0) = BPP;
	MLC32(MLCVSTRIDE0) = BPP*FB_VIRTUAL_XRES;
	MLC32(MLCLEFTRIGHT0) = ((xoff<<LEFT)|((FB_VIRTUAL_XRES-1+xoff)<<RIGHT));
	MLC32(MLCTOPBOTTOM0) = ((yoff<<TOP)|((FB_VIRTUAL_YRES-1+yoff)<<BOTTOM));
	MLC32(MLCCONTROL0) &= ~(0xFFFF<<FORMAT);
	MLC32(MLCCONTROL0) |= (MLC_FORMAT<<FORMAT);
	MLC32(MLCCONTROL0) |= (1<<DIRTYFLAG);

	/* turn on Layer 0 */

	BIT_SET(MLC32(MLCCONTROL0), PALETTEPWD); /* power up */
	BIT_CLR(MLC32(MLCCONTROL0), PALETTESLD); /* unsleep */
	BIT_SET(MLC32(MLCCONTROL0), LAYERENB);	 /* enable */
	BIT_SET(MLC32(MLCCONTROL0), DIRTYFLAG);	 /* apply */

	/* turn on the MLC */

	BIT_SET(MLC32(MLCCONTROLT), PIXELBUFFER_PWD); /* power up */
	BIT_SET(MLC32(MLCCONTROLT), PIXELBUFFER_SLD); /* unsleep */
	BIT_SET(MLC32(MLCCONTROLT), MLCENB);          /* enable */
	BIT_SET(MLC32(MLCCONTROLT), DITTYFLAG);       /* apply */
}
