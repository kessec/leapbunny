/* screens.c: Some screen data
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <mach/common.h>
#include "include/display.h"
#include "include/ram.h"
#include "include/rle.h"

const u32 __attribute__((section(".screens"))) ATTENTION_NEEDED[] = {
	#include "ATTENTION_NEEDED.32.rle.h"
};

const u32 __attribute__((section(".screens"))) DOWNLOAD_IN_PROGRESS[] = {
	#include "DOWNLOAD_IN_PROGRESS.32.rle.h"
};

const u32 __attribute__((section(".screens"))) LOW_BATTERY[] = {
	#include "LOW_BATTERY.32.rle.h"
};

const u32 __attribute__((section(".screens"))) VISIT[] = {
	#include "VISIT.32.rle.h"
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

void show_visit ()
{
	rle_decode (VISIT, (u32 *)get_frame_buffer_addr());
}
