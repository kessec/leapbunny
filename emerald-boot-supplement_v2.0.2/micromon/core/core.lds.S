/* vim: set ts=4 sw=4 noexpandtab
 *
 * Copyright (C) 2011 Jeff Kent <jakent@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "startup.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
	. = 0x0;
	. = ALIGN(4);
	.startup : { startup.o(.text .rodata .data) }

	. += MICROMON_START;
	. = ALIGN(4);
	.text : AT(ADDR(.startup) + SIZEOF(.startup))
	{
		/* these two objects must fit in the first 512 bytes */
		loader.o(.text .rodata .data)
		uart.o(.text .rodata .data)

		*(.text)
	}

	. = ALIGN(4);
	.rodata : { *(.rodata) }
	. = ALIGN(4);
	.data : { *(.data) }
	/* since our code is not position independent, we have no need
     * for global offset tables */
	/*. = ALIGN(4);
	.got : { *(.got) }*/

	. = BSS_START;
	. = ALIGN(4);
	__bss_start__ = .;
	.bss : { *(.bss) }
	__bss_end__ = .;

	. = STACK_END;
	. = ALIGN(4);
	__stack_end__ = .;
	. = STACK_START;
	. = ALIGN(4);
	__stack_start__ = .;

	/* discarded to unclutter map; should be removed if using
    /* mthumb-interwork */
	/DISCARD/ :	{ *(.glue_7 .glue_7t .vfp11_veneer .v4_bx) }
}
