/* display.h  -- display API
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This file contains prototypes for three functions:
 *      display_init()          resets and configures the LCD interface, 
 *                              then calls dpc_init() to configure the
 *                              display controller and mlc_init() to configure
 *                              multi-layer controller      
 *      display_backlight()     does what necessary to turn on the backlight
 *      zero_display_initd()    zeroes the variable the indicates whether or
 *                              not display_init() has already configured
 *                              the LCD interface, the display controller,
 *                              and the multi-layer controller.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_DISPLAY_H
#define LIGHTNING_BOOT_DISPLAY_H

int nt39016_detect(void);
void nt39016_init(void);
int hx8238_detect(void);
void hx8238_init(void);
int ili9322_detect(void);
void ili9322_init(void);

struct display_module {
	char *name;
	int (*detect)(void);
	void (*init)(void);
	u16 thb;
	u16 thf;
	u16 tvb;
	u16 tvf;
};

struct display_module *display_setup(void);
void display_init(void);
void display_backlight(int board_id);

#endif /* LIGHTNING_BOOT_DISPLAY_H */
