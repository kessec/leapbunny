/* dpc.c -- Basic Display Controller (DPC) driver for displaying boot splash
 *          screen.
 * 
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <board.h>
#include <common.h>
#include <base.h>
#include <dpc.h>

#define DPC16(x)	REG16(DPC_BASE+x)
#define DPC32(x)	REG32(DPC_BASE+x)

/* DPC settings */

#define DPC_VIDEO_FORMAT	3 /* RGB888 */

#define DPC_CLK0_SRC		1	/* use PLL1 */
#define DPC_CLK0_DIV		23	/* desired pad clock: 6.0 MHz */
#define DPC_CLK0_DELAY		0

#define DPC_CLK1_SRC		7	/* use clock generator 0 */
#define DPC_CLK1_DIV		0
#define DPC_CLK1_DELAY		0

#define DPC_PAD_CLK		3	/* inverted VCLK2 */
#define DPC_HSYNC_SWIDTH	2
#define DPC_VSYNC_SWIDTH	1

/* DPC registers as offsets from DPC_BASE */

#define DPCHTOTAL		0x07C
#define DPCHSWIDTH		0x07E
#define DPCHASTART		0x080
#define DPCHAEND		0x082
#define DPCVTOTAL		0x084
#define DPCVSWIDTH		0x086
#define DPCVASTART		0x088
#define DPCVAEND		0x08A
#define DPCCTRL0		0x08C
#define DPCCTRL1		0x08E
#define DPCCTRL2		0x098
#define DPCVSEOFFSET		0x09A
#define DPCVSSOFFSET		0x09C
#define DPCEVSEOFFSET		0x09E
#define DPCEVSSOFFSET		0x0A0
#define DPCDELAY0		0x0A2
#define DPCCLKENB		0x1C0
#define DPCCLKGEN0		0x1C4
#define DPCCLKGEN1		0x1C8

/* DPC CONTROL 0 REGISTER (DPCCTRL0) */
#define DPCENB		15
#define ENCENB		14
#define DACENB		13
#define RGBMODE		12
#define INTPEND		10
#define SCANMODE	9
#define SEAVENB		8
#define DELAYRGB	4
#define POLFIELD	2
#define POLVSYNC	1
#define POLHSYNC	0

/* DPC CONTROL 1 REGISTER (DPCCTRL1) */
#define SWAPRB		15
#define YCRANGE		13
#define FORMAT		8
#define YCORDER		6
#define BDITHER		4
#define GDITHER		2
#define RDITHER		0

/* DPC CONTROL 2 REGISTER (DPCCTRL2) */
#define PADCLKSEL	0

/* DPC CLOCK GENERATION ENABLE REGISTER (DPCCLKENB) */
#define PCLKMODE	3
#define CLKGENENB	2

/* DPC CLOCK GENERATION CONTROL 0 REGISTER (DPCCLKGEN0) */
#define OUTCLKENB	15
#define OUTCLKDELAY0	12
#define CLKDIV0		4
#define CLKSRCSEL0	1
#define OUTCLKINV0	0

/* DPC CLOCK GENERATION CONTROL 1 REGISTER (DPCCLKGEN1) */
#define OUTCLKDELAY1	12
#define CLKDIV1		4
#define CLKSRCSEL1	1
#define OUTCLKINV1	0

/* DPC SYNC DELAY 0 REGISTER (DPCDELAY0) */
#define DELAYDE		8
#define DELAYVS		4
#define DELAYHS		0

void dpc_enable(void)
{
	if (!(DPC16(DPCCTRL0) & (1<<DPCENB)))
		BIT_SET(DPC16(DPCCTRL0), DPCENB);
}

void dpc_init(const struct display_module *disp)
{
	u32 tmp;

	/* set PCLK to run only when CPU accesses it */
	BIT_CLR(DPC32(DPCCLKENB), PCLKMODE);

	/* set up DPC Clock 0 */

	tmp = DPC32(DPCCLKGEN0);

	tmp &= ~((7<<CLKSRCSEL0)|(0x3F<<CLKDIV0)|(3<<OUTCLKDELAY0));
	tmp |= (DPC_CLK0_SRC<<CLKSRCSEL0);
	tmp |= ((0x3F & DPC_CLK0_DIV)<<CLKDIV0);
	tmp |= (DPC_CLK0_DELAY<<OUTCLKDELAY0);
	BIT_SET(tmp, OUTCLKINV0); /* invert the clock */
	BIT_CLR(tmp, OUTCLKENB); /* don't connect to pin */
	DPC32(DPCCLKGEN0) = tmp;

	/* set up DPC Clock 1 */

	tmp = DPC32(DPCCLKGEN1);
	tmp &= ~((7<<CLKSRCSEL1)|(0x3F<<CLKDIV1)|(3<<OUTCLKDELAY1));
	tmp |= (DPC_CLK1_SRC<<CLKSRCSEL1);
	tmp |= ((0x3F & DPC_CLK1_DIV)<<CLKDIV1);
	tmp |= (DPC_CLK1_DELAY<<OUTCLKDELAY1);
	BIT_SET(tmp, OUTCLKINV1); /* invert the clock */
	DPC32(DPCCLKGEN1) = tmp;

	/* enable the clock generator */
	BIT_SET(DPC32(DPCCLKENB), CLKGENENB);

	/* set up control registers */

	BIT_CLR(DPC16(DPCCTRL0), INTPEND);
	/* turn off interlace, field invert, and embedded sync */
	DPC16(DPCCTRL0) &= ~((1<<SCANMODE)|(1<<POLFIELD)|(1<<SEAVENB)|
				(0xF<<DELAYRGB));
	/* use RGB mode */
	BIT_SET(DPC16(DPCCTRL0), RGBMODE);
	
	/* active-low sync */
	BIT_CLR(DPC16(DPCCTRL0), POLHSYNC);
	BIT_CLR(DPC16(DPCCTRL0), POLVSYNC);

	DPC16(DPCCTRL1) &= ~(0xAFFF); /* clear all but reserved bits */
	DPC16(DPCCTRL1) |= (DPC_VIDEO_FORMAT<<FORMAT);

	DPC16(DPCCTRL2) &= ~(3<<PADCLKSEL);
	DPC16(DPCCTRL2) |= (DPC_PAD_CLK<<PADCLKSEL);

	/* set up horizonal sync */

	DPC16(DPCHTOTAL)  = DPC_HSYNC_SWIDTH + disp->thf +
		disp->thb + disp->xres - 1;
	DPC16(DPCHSWIDTH) = DPC_HSYNC_SWIDTH - 1;
	DPC16(DPCHASTART) = DPC_HSYNC_SWIDTH + disp->thb - 1;
	DPC16(DPCHAEND)   = DPC_HSYNC_SWIDTH + disp->thb + disp->xres - 1;

	/* set up vertical sync */

	DPC16(DPCVTOTAL)  = DPC_VSYNC_SWIDTH + disp->tvf +
		disp->tvb + disp->yres - 1;
	DPC16(DPCVSWIDTH) = DPC_VSYNC_SWIDTH - 1;
	DPC16(DPCVASTART) = DPC_VSYNC_SWIDTH + disp->tvb - 1;
	DPC16(DPCVAEND)   = DPC_VSYNC_SWIDTH + disp->tvb + disp->yres - 1;

	/* set up delays */
	DPC16(DPCDELAY0) = ((7<<DELAYHS)|(7<<DELAYVS)|(7<<DELAYDE));

	/* Set vsync offset */

	DPC16(DPCVSEOFFSET)  = 1;
	DPC16(DPCVSSOFFSET)  = 1;
	DPC16(DPCEVSEOFFSET) = 1;
	DPC16(DPCEVSSOFFSET) = 1;
}
