/* bbtest.h - test code for bad block handling / kernel loading
 *
 * Copyright 2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BBTEST_H__
#define __BBTEST_H__

#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
/* Do a kernel bad bloack loading test based on button sequences:
 * 1) LS+RS+UP: force one or more blocks of the kernel partition to be bad 
 * 2) LS+RS+DOWN: erase the entire kernel partition, even the blocks marked bad
 */ 
void do_kernel_bad_block_test(struct buttons_state *buttons);

#else

#define do_kernel_bad_block_test(b)

#endif

#endif /* __BBTEST_H__ */
