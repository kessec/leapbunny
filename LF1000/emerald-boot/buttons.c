/* buttons.c -- Button mapping and lookup.
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <board.h>
#include <global.h>
#include <gpio.h>
#include <gpio_hal.h>
#include <gpio_map.h>  /* GPIO button mappings */
#include <buttons.h>
#include <debug.h>

void buttons_get_state(struct buttons_state *b)
{
	global_var * gptr = get_global();

	switch(gptr->board_id) {
	case LF1000_BOARD_DEV:
		b->a = !gpio_get_val(DEV_BUTTON_A_PORT, DEV_BUTTON_A_PIN);
		b->b = !gpio_get_val(DEV_BUTTON_B_PORT, DEV_BUTTON_B_PIN);
		b->rs = !gpio_get_val(DEV_SHOULDER_RIGHT_PORT, DEV_SHOULDER_RIGHT_PIN);
		b->ls = !gpio_get_val(DEV_SHOULDER_LEFT_PORT, DEV_SHOULDER_LEFT_PIN);
		b->p = !gpio_get_val(DEV_BUTTON_PAUSE_PORT, DEV_BUTTON_PAUSE_PIN);
		b->hint = !gpio_get_val(DEV_BUTTON_HINT_PORT, DEV_BUTTON_HINT_PIN);
#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
		b->up   = !gpio_get_val(DEV_DPAD_UP_PORT, DEV_DPAD_UP_PIN);
		b->down = !gpio_get_val(DEV_DPAD_DOWN_PORT, DEV_DPAD_DOWN_PIN);
#endif
		break;
	case LF1000_BOARD_DIDJ:
	case LF1000_BOARD_DIDJ_09:
		b->a = !gpio_get_val(DIDJ_BUTTON_A_PORT, DIDJ_BUTTON_A_PIN);
		b->b = !gpio_get_val(DIDJ_BUTTON_B_PORT, DIDJ_BUTTON_B_PIN);
		b->rs = !gpio_get_val(DIDJ_SHOULDER_RIGHT_PORT, DIDJ_SHOULDER_RIGHT_PIN);
		b->ls = !gpio_get_val(DIDJ_SHOULDER_LEFT_PORT, DIDJ_SHOULDER_LEFT_PIN);
		b->p = !gpio_get_val(DIDJ_BUTTON_PAUSE_PORT, DIDJ_BUTTON_PAUSE_PIN);
		b->hint = !gpio_get_val(DIDJ_BUTTON_HINT_PORT, DIDJ_BUTTON_HINT_PIN);
#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
		b->up   = !gpio_get_val(DIDJ_DPAD_UP_PORT, DIDJ_DPAD_UP_PIN);
		b->down = !gpio_get_val(DIDJ_DPAD_DOWN_PORT, DIDJ_DPAD_DOWN_PIN);
#endif
		break;
	case LF1000_BOARD_ACORN:
		b->a = !gpio_get_val(ACORN_BUTTON_A_PORT, ACORN_BUTTON_A_PIN);
		b->b = !gpio_get_val(ACORN_BUTTON_B_PORT, ACORN_BUTTON_B_PIN);
		b->rs = !gpio_get_val(ACORN_SHOULDER_RIGHT_PORT, ACORN_SHOULDER_RIGHT_PIN);
		b->ls = !gpio_get_val(ACORN_SHOULDER_LEFT_PORT, ACORN_SHOULDER_LEFT_PIN);
		b->p = !gpio_get_val(ACORN_BUTTON_PAUSE_PORT, ACORN_BUTTON_PAUSE_PIN);
		b->hint = !gpio_get_val(ACORN_BUTTON_HINT_PORT, ACORN_BUTTON_HINT_PIN);
#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
		b->up   = !gpio_get_val(ACORN_DPAD_UP_PORT, ACORN_DPAD_UP_PIN);
		b->down = !gpio_get_val(ACORN_DPAD_DOWN_PORT, ACORN_DPAD_DOWN_PIN);
#endif
		break;
	case LF1000_BOARD_EMERALD_POP:
	case LF1000_BOARD_EMERALD_NOTV_NOCAP:
	case LF1000_BOARD_EMERALD_TV_NOCAP:
	case LF1000_BOARD_EMERALD_NOTV_CAP:
	case LF1000_BOARD_EMERALD_SAMSUNG:
		b->a = !gpio_get_val(EMERALD_BUTTON_A_PORT, EMERALD_BUTTON_A_PIN);
		b->b = !gpio_get_val(EMERALD_BUTTON_B_PORT, EMERALD_BUTTON_B_PIN);
		b->rs = !gpio_get_val(EMERALD_SHOULDER_RIGHT_PORT, EMERALD_SHOULDER_RIGHT_PIN);
		b->ls = !gpio_get_val(EMERALD_SHOULDER_LEFT_PORT, EMERALD_SHOULDER_LEFT_PIN);
		b->p = !gpio_get_val(EMERALD_BUTTON_PAUSE_PORT, EMERALD_BUTTON_PAUSE_PIN);
		b->hint = !gpio_get_val(EMERALD_BUTTON_HINT_PORT, EMERALD_BUTTON_HINT_PIN);
#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
		b->up   = !gpio_get_val(EMERALD_DPAD_UP_PORT, EMERALD_DPAD_UP_PIN);
		b->down = !gpio_get_val(EMERALD_DPAD_DOWN_PORT, EMERALD_DPAD_DOWN_PIN);
#endif
		break;
	case LF1000_BOARD_K2:
		b->a = !gpio_get_val(K2_BUTTON_A_PORT, K2_BUTTON_A_PIN);
		b->b = !gpio_get_val(K2_BUTTON_B_PORT, K2_BUTTON_B_PIN);
		b->rs = !gpio_get_val(K2_SHOULDER_RIGHT_PORT, K2_SHOULDER_RIGHT_PIN);
		b->ls = !gpio_get_val(K2_SHOULDER_LEFT_PORT, K2_SHOULDER_LEFT_PIN);
		// Make red onto both hint and pause
		b->p = !gpio_get_val(K2_BUTTON_RED_PORT, K2_BUTTON_RED_PIN);
		b->hint = !gpio_get_val(K2_BUTTON_RED_PORT, K2_BUTTON_RED_PIN);
#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
		b->up   = !gpio_get_val(K2_DPAD_UP_PORT, K2_DPAD_UP_PIN);
		b->down = !gpio_get_val(K2_DPAD_DOWN_PORT, K2_DPAD_DOWN_PIN);
#endif
		break;
	case LF1000_BOARD_MADRID:
	case LF1000_BOARD_MADRID_LFP100:
	case LF1000_BOARD_MADRID_POP:
		//FIXME: missing some boot buttons, just zero them for now
		b->a = !gpio_get_val(MADRID_BUTTON_VOLUMEDOWN_PORT, MADRID_BUTTON_VOLUMEDOWN_PIN);
		b->b = !gpio_get_val(MADRID_BUTTON_VOLUMEUP_PORT, MADRID_BUTTON_VOLUMEUP_PIN);
		b->rs = !gpio_get_val(MADRID_DPAD_RIGHT_PORT, MADRID_DPAD_RIGHT_PIN);
		b->ls = !gpio_get_val(MADRID_BUTTON_VOLUMEDOWN_PORT, MADRID_BUTTON_VOLUMEDOWN_PIN);
		b->p = 0;
		b->hint = !gpio_get_val(MADRID_BUTTON_ESC_PORT, MADRID_BUTTON_ESC_PIN);
#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
		b->up   = !gpio_get_val(MADRID_DPAD_UP_PORT, MADRID_DPAD_UP_PIN);
		b->down = !gpio_get_val(MADRID_DPAD_DOWN_PORT, MADRID_DPAD_DOWN_PIN);
#endif
		break;
	default:
		db_puts("unknown board type\n");
		b->a = b->b = b->ls = b->rs = b->p = b->hint = 0;
#ifdef TEST_BOOT_WITH_KERNEL_BAD_BLOCKS
		b->up   = b->down = 0;
#endif
		break;
	}
}
