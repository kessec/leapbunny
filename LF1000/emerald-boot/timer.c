/* timer.c -- simple timer for profiling
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * Note: this driver is designed to utilize the CPU's timer hardware to enable
 * simple profiling operations for optimizing bootloader code.  As such, its
 * routines work as a 'stopwatch'.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <base.h>

/* Timer n Registers (offsets from TIMERn_BASE) */
#define TMRCOUNT        0x00
#define TMRCONTROL      0x08
#define TMRCLKENB       0x40
#define TMRCLKGEN       0x44

/* Timer Control Register n (TMRCONTROLn) */
#define LDCNT           6
#define RUN             3
#define SETCLK          0

/* Timer Clock Generation Enable Register n (TMRCLKENBn) */
#define TCLKGENENB      2 /* added T to differentiate from others */

/* Timer Clock Generation Control Register n (TMRCLKGENn) */
#define TCLKDIV         4 /* added T to differentiate from others */
#define TCLKSRCSEL      1

/* 
 * settings 
 */

#define TIMER_BASE	TIMER0_BASE
#define TIMER_CLK_SRC	1
#define TIMER_PRES	17			/* divide by 18 */
#define TIMER_CLK	2			/* divide by 8 */

#define TIMER32(r)	REG32(TIMER_BASE+r)

/*
 * Timer Clock Rate:
 *
 * TIMER_SRC_RATE/(TIMER_PRES+1)/(TIMER_CLK+1) =
 *
 * 147461538/18/8 = 1024038 Hz, ~ 1.024MHz, ~1us tick
 */

#ifdef DEBUG_STOPWATCH

void timer_init(void)
{
	/* make sure the timer is stopped */
	BIT_CLR(TIMER32(TMRCONTROL), RUN);

	/* set up the clock */
	TIMER32(TMRCONTROL) &= ~(3<<SETCLK);
	TIMER32(TMRCONTROL) |= (TIMER_CLK<<SETCLK);
	TIMER32(TMRCLKGEN) = (TIMER_PRES<<TCLKDIV)|(TIMER_CLK_SRC<<TCLKSRCSEL);
	BIT_SET(TIMER32(TMRCLKENB), TCLKGENENB);
}

/* start the stopwatch */
void timer_start(void)
{
	/* make sure the timer is stopped */
	BIT_CLR(TIMER32(TMRCONTROL), RUN);

	/* zero out the timer */
	TIMER32(TMRCOUNT) = 0;

	/* run the timer */
	BIT_SET(TIMER32(TMRCONTROL), RUN);
}

/* stop the stopwatch, and return the time */
u32 timer_stop(void)
{
	/* stop the timer */
	BIT_CLR(TIMER32(TMRCONTROL), RUN);
	/* get access to counter */
	BIT_SET(TIMER32(TMRCONTROL), LDCNT);

	return TIMER32(TMRCOUNT);
}

#endif /* DEBUG_STOPWATCH */
