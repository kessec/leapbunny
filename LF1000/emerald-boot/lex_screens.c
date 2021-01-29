/* screens.c: Some screen data
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>
#include <common.h>
#include <display.h>
#include <global.h>
#include <ram.h>
#include <rle.h>

const u32 __attribute__((section(".screens"))) ATTENTION_NEEDED[] = {
	#include "Lex-ATTENTION_NEEDED.32.rle.h"
};

const u32 __attribute__((section(".screens"))) DOWNLOAD_IN_PROGRESS[] = {
	#include "Lex-DOWNLOAD_IN_PROGRESS.32.rle.h"
};

const u32 __attribute__((section(".screens"))) LOW_BATTERY[] = {
	#include "Lex-LOW_BATTERY.32.rle.h"
};

const u32 __attribute__((section(".screens"))) VISIT[] = {
	#include "Lex-VISIT.32.rle.h"
};

const u32 __attribute__((section(".screens"))) VISIT_FR[] = {
	#include "Lex-VISIT_FR.32.rle.h"
};

const u32 __attribute__((section(".screens"))) VISIT_ES[] = {
	#include "Lex-VISIT_ES.32.rle.h"
};

void show_attention_needed ()
{
	rle_decode (ATTENTION_NEEDED, (u32 *)get_frame_buffer_addr());
}

void show_download_in_progress ()
{
	rle_decode (DOWNLOAD_IN_PROGRESS, (u32 *)get_frame_buffer_addr());
}

void show_low_battery ()
{
	rle_decode (LOW_BATTERY, (u32 *)get_frame_buffer_addr());
}

#define kOffsetLocale	104
#define kSizeLocale		8

void show_visit ()
{
	char locale[kSizeLocale];
	u32 RAM = calc_SDRAM_ADDRESS();
	u32 NOR = RAM ? 0x0007e000 : 0x8007e000;
	memcpy (locale, (char *)NOR+kOffsetLocale, kSizeLocale);

	const u32 *visit_screen = VISIT;
	if(!strncmp(locale, "en-", 3))
		visit_screen = VISIT;
	else if(!strncmp(locale, "fr-", 3))
		visit_screen = VISIT_FR;
	else if(!strncmp(locale, "es-", 3))
		visit_screen = VISIT_ES;

	rle_decode (visit_screen, (u32 *)get_frame_buffer_addr());
}
