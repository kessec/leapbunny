/* copyNor.c -- copy bootstrap code from NOR to SDRam for faster execution.
 *
 * Copyright 2007-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "include/autoconf.h"   /* for partition info */
#include "include/mach-types.h" /* for machine info */
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/gpio_hal.h>
#include <mach/gpio.h>
#include "include/debug.h"

/* NOTE: _copy_start and _copy_end (defined in emerald.lds) must be
 *		 4-byte aligned.
 */
extern unsigned int _copy_start;
extern unsigned int _copy_end;

void nor_bootstrap(void)
{
	unsigned int *src, *dst;

	/* Ping oscilloscope */
	if (1)
	{
#define GPIO32(x)       REG32(LF1000_GPIO_BASE+x)
		u32 reg, pin, tmp;
		reg = GPIOAALTFN0 + GPIO_PORT_B*0x40;
		pin = GPIO_PIN8*2;
		tmp = GPIO32(reg);
		tmp &= ~(3<<pin);
		tmp |= ((GPIO_GPIOFN)<<pin);
		GPIO32(reg) = tmp;

		pin = GPIO_PIN8;
		reg = GPIOAOUT + GPIO_PORT_B*0x40;
		BIT_SET(GPIO32(reg), pin);
		reg = GPIOAOUTENB + GPIO_PORT_B*0x40;
		BIT_SET(GPIO32(reg), pin);
		reg = GPIOAOUT + GPIO_PORT_B*0x40;
		BIT_CLR(GPIO32(reg), pin);
	}

	/* 'latch' the system power so that user may let go of power switch */

	/* enable access to Alive GPIO */
	REG32(LF1000_ALIVE_BASE+ALIVEPWRGATEREG) = 1;
	/* pull VDDPWRON high by setting the flip-flop */
	BIT_SET(REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG), VDDPWRONSET);
	/* reset flip-flop to latch in */
	REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG) = 0;
	REG32(LF1000_ALIVE_BASE+ALIVEGPIORSTREG) = 0;

	db_init();

	/* Note: Turn off this option if you are testing emerald-boot by
	 *       forcing it into address 0 via u-boot or some other means like
	 *       JTAG.  */
#ifdef SELF_BOOTSTRAP
	/* copy NOR to SDRAM */
	for (src=(unsigned int *) & _copy_start,
	     dst=(unsigned int *)0x80000000;
		((unsigned int)src) < ((unsigned int)& _copy_end); dst++, src++) {
		*dst = *src;
	}
	db_puts("copied bootstrap\n");
#endif
}
