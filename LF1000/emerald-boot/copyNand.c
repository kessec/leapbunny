/* copyNand.c -- wrapper for nand_read() to enable self-bootstrapping 
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
#include <mach/clkpwr.h>
#include <mach/nand.h>
#include "include/nand.h"
#include "include/debug.h"

#ifdef SELF_BOOTSTRAP
/* Bootstrap settings: these are determined by the hardware.  
 * The LF1000 CPU's NAND Boot state machine loads 0x800 (one Large Block
 * NAND page) of data for us.  We need to load the rest,
 * which is _copy_end - _copy_start minus what's already loaded. 
 */
#define BOOTSTRAP_SIZE	0x0800
u32 GetCopySize();  // Returns the value of _copy_end.
#endif

void nand_bootstrap()
{
	const u32 page_size = 2048;
	const u32 page_shift = 11;

#if 0   //----------------------------------------------------------------------
        u32 numBytes;
        u32 plusPageSize;
        u32 copySize;

       // when calculated here, wrong values are computed (0: 7FF: 0).
        // The codes access the values of _copy_end and _copy_start through
        // a table in the .got segment, which is located after the .text
        // segment.  It is not in the first 0x800 bytes of the binary
        // image, so it's not in SDRam when this code is executed.
        // Therefore incorrect values of _copy_end and _copy_start are
        // found and used in the calculation.
        numBytes = (u32)_copy_end - (u32)_copy_start;
        plusPageSize = numBytes + page_size - 1;
        copySize     = plusPageSize & ~(page_size - 1);

// Therefore we added GetCopySize() in startup.S and call it to calculate
// the 3rd parameter passed to nand_read().
#endif  //----------------------------------------------------------------------

	/* Ping oscilloscope */
#if 1
    *((volatile unsigned int*)0xc000a060) &= ~(0x0000000f);
    *((volatile unsigned int*)0xc000a044) |= 0x00000003;
	*((volatile unsigned int*)0xc000a040) &= ~(0x00000001); // LED 1 on
	*((volatile unsigned int*)0xc000a040) &= ~(0x00000002); // LED 2 on
#endif
#if 0   // what is/was the purpose of this code?
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
#endif
	/* 'latch' the system power so that user may let go of power switch */

	/* enable access to Alive GPIO */
	REG32(LF1000_ALIVE_BASE+ALIVEPWRGATEREG) = 1;
	/* pull VDDPWRON high by setting the flip-flop */
	BIT_SET(REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG), VDDPWRONSET);
	/* reset flip-flop to latch in */
	REG32(LF1000_ALIVE_BASE+ALIVEGPIOSETREG) = 0;
	REG32(LF1000_ALIVE_BASE+ALIVEGPIORSTREG) = 0;

	/* Note: Turn off this option if you are testing emerald-boot by
	 *       forcing it into address 0 via u-boot or some other means like
	 *       JTAG. */
#ifdef SELF_BOOTSTRAP
	/* load the rest of the boot module (hardware loaded 2K) */
        // Call nand_read_bootstrap() to read (u32)_copy_end - BOOTSTRAP_SIZE
        // (rounded up to the nearest multiple of the page size) bytes
        // and store them in SDRam, starting at address BOOTSTRAP_SIZE.
	nand_read_bootstrap((u32 *)BOOTSTRAP_SIZE, BOOTSTRAP_SIZE, 
			    (GetCopySize() - BOOTSTRAP_SIZE + page_size - 1)
			    & ~(page_size - 1),
			    page_shift);
#endif
	/* we're in SDRAM, now can call all functions */
	db_init();

#ifdef SELF_BOOTSTRAP
    db_puts("GetCopySize(): "); db_int( GetCopySize() ); 
    db_puts("; 3rd arg: "); 
    db_int( (GetCopySize() - BOOTSTRAP_SIZE + page_size - 1)
            & ~((page_size - 1)) ); 
#endif
    db_putchar('\n');
}
