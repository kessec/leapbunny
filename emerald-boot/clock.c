/* clock.c  -- LF1000 clock initialization
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <base.h>
#include <board.h>

#define CPU_PLL		0
#define BCLK_PLL	0
#define CPU_DIV		1
#define HCLK_DIV	3
#define BCLK_DIV	3

#define CLK32(r)	REG32(CLKPWR_BASE+r)

#define CLKMODEREG      0x000
#define PLLSETREG0      0x004
#define PLLSETREG1      0x008
#define PWRMODE         0x07C
#define INTPENDSPAD     0x058

/* PLL0 Setting Register (PLLSETREG0) */
#define PDIV_0          18
#define MDIV_0          8
#define SDIV_0          0

/* PLL1 Setting Register (PLLSETREG1) */
#define PDIV_1          18
#define MDIV_1          8
#define SDIV_1          0

/* Power Mode Control Register (PWRMODE) */
#define CHGPLL          15
#define GPIOSWRSTENB    13
#define SWRST           12
#define LASTPWRMODE     4
#define CURPWRMODE      0

/* Clock Mode Register (CLKMODEREG) */
#define PLLPWDN1        30
#define CLKSELBCLK      24
#define CLKDIV1BCLK     20
#define CLKDIV2CPU0     6
#define CLKSELCPU0      4
#define CLKDIVCPU0      0

/* Interrupt Pending & Scratch Pad Register (INTPENDSPAD) */
#define BATFW           14
#define GPIORESETW      13
#define WATCHDOGRSTW    12
#define POWERONRSTW     11

#define PLL_REG(m, p, s) \
	(((unsigned int)(m) << MDIV_0) | \
	 ((unsigned int)(p) << PDIV_0) | \
	 ((unsigned int)(s) << SDIV_0))

#define PLL_CLOCK0	PLL_REG(PLL0_M, PLL0_P, PLL0_S)

/* PLL1 (bus clock) 147000000 Hz */
/* #define PLL_CLOCK1	PLL_REG(196, 9, 2) */
/* PLL1 (bus clock) 144000000 Hz */
#define PLL_CLOCK1	PLL_REG( 64, 3, 2)

/*
 * Perform LF1000 clock setup
 */
int clock_init(void)
{
	u32 tmp;

	/* calculate bus clock divider */
	tmp = REG32(CLKPWR_BASE+CLKMODEREG);

	tmp &= ~((0x3<<CLKSELCPU0) |
		 (0xF<<CLKDIVCPU0) |
		 (0xF<<CLKDIV2CPU0)|
		 (0x3<<CLKSELBCLK) |
		 (0xF<<CLKDIV1BCLK));

	tmp |= ((CPU_PLL<<CLKSELCPU0)		|
		((CPU_DIV-1)<<CLKDIVCPU0)	|
		((HCLK_DIV-1)<<CLKDIV2CPU0)	|
		(BCLK_PLL<<CLKSELBCLK)		|
		((BCLK_DIV-1)<<CLKDIV1BCLK));

	/* leave settings alone if already setup */
	if (REG32(CLKPWR_BASE+PLLSETREG0) == PLL_CLOCK0 &&
	    REG32(CLKPWR_BASE+PLLSETREG1) == PLL_CLOCK1 &&
	    REG32(CLKPWR_BASE+CLKMODEREG) == tmp) {
		return 0;
	}

	/* set the PLLs */
	REG32(CLKPWR_BASE+PLLSETREG0) = PLL_CLOCK0;
	REG32(CLKPWR_BASE+PLLSETREG1) = PLL_CLOCK1;

	REG32(CLKPWR_BASE+CLKMODEREG) = tmp;

	/* apply settings */
	BIT_SET(CLK32(PWRMODE), CHGPLL);

	/* wait for PLLs to stabalize */
	while (IS_SET(CLK32(PWRMODE), CHGPLL));
	return 1;
}
