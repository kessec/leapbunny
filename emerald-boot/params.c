/* params.c -- buffer where kernel parameter structures are stored
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/*
 * This is a place for the atags structure built by build_params() in
 * main.c This must be located in the first 16K of RAM, but not in the
 * first 2K!!  The .text section dominates the first 20+K, so we have
 * to squeeze in there.
 *
 * In the linker (see Makefile) load params.o after bootstrap.o and
 * nand.o, which must be in first 2K for NAND to boot, since the
 * LF1000 only loads in 2K before jumping to 0.  The NOR does not have
 * this constraint.
 *
 * NOTE: Because my_kernel_params[] is in the text segment, and because the
 *       code writes into my_kernel_params[], the text segment must be copied
 * to SDRam before code writes into my_kernel_params[].
 */
#include <common.h>
u32 __attribute__((section(".text"))) my_kernel_params[0x800/sizeof(u32)];
