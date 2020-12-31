/* clock.h  -- LF1000 clock initialization
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_CLOCK_H
#define LIGHTNING_BOOT_CLOCK_H

#include "autoconf.h"
#include "mach-types.h" /* for machine info */
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/clkpwr.h>

#define CLK32(r)	REG32(LF1000_CLKPWR_BASE+r)

#define PLL_REG(m, p, s) \
	(((unsigned int)(m) << MDIV_0) | \
	 ((unsigned int)(p) << PDIV_0) | \
	 ((unsigned int)(s) << SDIV_0))

#if defined   CONFIG_CPU_SPEED_532480000 /* PLL0 (CPU clock) 532480000 Hz */
#define PLL_CLOCK0	PLL_REG(355, 18, 0)

#elif defined CONFIG_CPU_SPEED_393216000 /* PLL0 (CPU clock) 393216000 Hz */
#define PLL_CLOCK0	PLL_REG(801, 55, 0)
#else
#error CPU SPEED not defined
#endif

/* PLL1 (bus clock) 147000000 Hz */
/* #define PLL_CLOCK1	PLL_REG(196, 9, 2) */
/* PLL1 (bus clock) 144000000 Hz */
#define PLL_CLOCK1	PLL_REG( 64, 3, 2)

/*
 * Perform LF1000 clock setup
 */
#if defined CPU_LF1000 && defined CONFIGURE_CLOCKS
static inline void clock_init(void)
{
	u32 tmp;

	/* set the PLLs */
	REG32(LF1000_CLKPWR_BASE+PLLSETREG0) = PLL_CLOCK0;
	REG32(LF1000_CLKPWR_BASE+PLLSETREG1) = PLL_CLOCK1;

	/* set bus clock divider */
	tmp = REG32(LF1000_CLKPWR_BASE+CLKMODEREG);

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

	REG32(LF1000_CLKPWR_BASE+CLKMODEREG) = tmp;

	/* apply settings */
	BIT_SET(CLK32(PWRMODE), CHGPLL);

	/* wait for PLLs to stabalize */
	while(IS_SET(CLK32(PWRMODE), CHGPLL));
}
#else
#define clock_init(...)
#endif

#endif /* LIGHTNING_BOOT_CLOCK_H */
