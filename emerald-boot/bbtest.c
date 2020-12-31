/* bbtest.c -- Bad-block test code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "include/autoconf.h"
#include "include/mach-types.h" /* for machine info */
#include <mach/platform.h>
#include <mach/common.h>
#include "include/board.h"
#include "include/nand.h"
#include "include/buttons.h"

#ifndef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
#error "Shouldn't be building bbtest.c"
#endif

// force one block of the nand to be bad
void force_bad_block_in_nand(unsigned int address);
// erase one block in nand, even if marked bad
void erase_block_in_nand(unsigned int address);

// force one or more blocks of the kernel partition to be bad
void force_bad_blocks_in_kernel_nand_partition()
{
	// here we mark the partition's first block bad
	force_bad_block_in_nand( BOOT0_ADDR(baseEBS));
	// here we mark the partition's second block bad
	force_bad_block_in_nand( BOOT0_ADDR(baseEBS) + NAND_EB_SIZE);
	// here we mark the partition's third block bad
	force_bad_block_in_nand( BOOT0_ADDR(baseEBS) + 2 * NAND_EB_SIZE);
	// here we mark the partition's fourth block bad
	// force_bad_block_in_nand( BOOT0_ADDR(baseEBS) + 3 * NAND_EB_SIZE);
	// here we mark the partition's fifth block bad
	force_bad_block_in_nand( BOOT0_ADDR(baseEBS) + 4 * NAND_EB_SIZE);
	// here we mark the partition's sixth block bad
	force_bad_block_in_nand( BOOT0_ADDR(baseEBS) + 5 * NAND_EB_SIZE);
	// here we mark the partition's seventh block bad
	force_bad_block_in_nand( BOOT0_ADDR(baseEBS) + 6 * NAND_EB_SIZE);
	// here we mark the partition's eighth block bad
	force_bad_block_in_nand( BOOT0_ADDR(baseEBS) + 7 * NAND_EB_SIZE);
}

// erase the entire kernel partition, even the blocks marked bad
void erase_all_blocks_in_kernel_nand_partition()
{
	unsigned int offset;

	for (offset = 0; offset < P2_SIZE; offset += NAND_EB_SIZE)
		erase_block_in_nand(BOOT0_ADDR(0) + offset);
}
void do_kernel_bad_block_test(struct buttons_state *buttons)
{
	if (buttons->ls && buttons->rs && buttons->up) {
		force_bad_blocks_in_kernel_nand_partition();
		return;
	}

	if (buttons->ls && buttons->rs && buttons->down) {
		erase_all_blocks_in_kernel_nand_partition();
	}
}
