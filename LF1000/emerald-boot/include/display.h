/* display.h  -- display API
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This file contains prototypes for three functions:
 *      display_init()          resets and configures the LCD interface, 
 *                              then calls dpc_init() to configure the
 *                              display controller and mlc_init() to configure
 *                              multi-layer controller      
 *      display_backlight()     does what necessary to turn on the backlight
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <common.h>

int nt39016_detect(void);
void nt39016_init(void);
int hx8238_detect(void);
void hx8238_init(void);
int ili9322_detect(void);
void ili9322_init(void);
int ili6480g2_detect(void);
void ili6480g2_init(void);

struct display_module {
#if 0	// 5apr11
	char *name;
	int (*detect)(void);
	void (*init)(void);
#endif	// 5apr11
	u16 xres;
	u16 yres;
	u16 thb;
	u16 thf;
	u16 tvb;
	u16 tvf;
#if 1	// 5apr11
	const char *name;
	int (*detect)(void);
	void (*init)(void);
#endif	// 5apr11
};


#if 1	// 5apr11

struct madrid_display_module {
	//char *name;
	//int (*detect)(void);
	//void (*init)(void);
	u16 xres;
	u16 yres;
	u16 thb;
	u16 thf;
	u16 tvb;
	u16 tvf;
	const char *name;
};

struct display_module * madrid_display_setup(struct madrid_display_module *pdm);
#endif


struct display_module *display_setup(void);
void display_init(void);
void display_backlight(void);

#endif /* __DISPLAY_H__ */
