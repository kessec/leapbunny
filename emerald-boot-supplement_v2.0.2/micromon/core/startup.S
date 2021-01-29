/* vim: set ts=8 sw=8 noexpandtab
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
.section .text
.arm
.align 0

.global _start
.org	0

_start:
	/* relocate to high memory */
	ldr	r0, =0
	ldr	r1, =CORE_START

.relocate_loop:
	ldmia	r0!, {r3-r10}
	stmia	r1!, {r3-r10}
	cmp	r0, #CORE_SIZE
	bne	.relocate_loop

	/* initialize stack */
	ldr	sp, =STACK_START

	/* jump to loader in thumb mode */
 	ldr	pc, =(loader+1)

.end
