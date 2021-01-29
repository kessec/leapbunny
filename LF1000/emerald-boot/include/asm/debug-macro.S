/* mach-lf1000/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 * Scott Esters <sesters@leapfrog.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#define IO_ADDRESS_OFF          0xf0000000
#define IO_ADDRESS(x)           (((x) & 0x00ffffff) + (((x) >> 4) & 0x0ff00000) + IO_ADDRESS_OFF)

#define LF1000_UART0_BASE		0xC0016000
/* TRSTATUS */
#define TRANSMITTER_EMPTY       2
#define TRANSMIT_BUFFER_EMPTY   1
#define RECEIVE_BUFFER_DATA_READY       0

#define TRSTATUS                0x08
#define THB                     0x10


	.macro  addruart,rx
	mrc     p15, 0, \rx, c1, c0
	tst     \rx, #1				@ MMU enabled?
	ldreq   \rx, =LF1000_UART0_BASE		@ System peripherals (phys address)
	ldrne   \rx, =IO_ADDRESS(LF1000_UART0_BASE) @ System peripherals (virt address)
	.endm

	.macro  senduart,rd,rx
	strb    \rd, [\rx, #THB]		@ Write Transmitter Holding Register
	.endm

        .macro  busyuart,rd,rx
1001:   ldr	\rd, [\rx, #TRSTATUS]		@ get TRANSMIT STATUS reg
        tst     \rd, #(1<<TRANSMITTER_EMPTY)	@ = 1 when tx complete
        beq	1001b
        .endm

	.macro  waituart,rd,rx
1001:   ldr	\rd, [\rx, #TRSTATUS]		@ get TRANSMIT STATUS reg
        tst     \rd, #(1<<TRANSMITTER_EMPTY)	@ = 1 when tx complete
        beq	1001b
        .endm
