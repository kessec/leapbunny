/* display.c -- Basic display driver: find the system's screen and set up
 * 		the display controller timings and the MLC and set an initial
 * 		backlight.
 * 
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <gpio.h>
#include <dpc.h>
#include <mlc.h>
#include <pwm.h>
#include <i2c.h>
#include <display.h>
#include <lfp100.h>
#include <board.h>
#include <global.h>
#include <debug.h>

/* Table of supported display modules and their DPC settings:  We will check
 * for the system display in the order given in this table.  Please note that
 * the order matters: some checks are performed by reading from registers and
 * those reads may affect some subsequent screen's controller due to the
 * varying protocols used.  Add your new screen to the right position in this
 * table such that it gets detected and doesn't mess up the others.  Set
 * MODULE_DEFAULT accordingly. */
static struct display_module modules[] = {
	{
		.name	= "ILI9322",
		.detect	= ili9322_detect,
		.init	= ili9322_init,
		.xres	= 320,
		.yres	= 240,
		.thb	= 36,
		.thf	= 50,
		.tvb	= 17,
		.tvf	= 4,
	},
	{
		.name	= "ILI6480G2",
		.detect = ili6480g2_detect,
		.init	= ili6480g2_init,
		.xres	= 480,
		.yres	= 272,
		.thb	= 40,
		.thf	= 5,
		.tvb	= 8,
		.tvf	= 8,
	},
	{
		.name	= "NT39016",
		.detect	= nt39016_detect,
		.init	= nt39016_init,
		.xres	= 320,
		.yres	= 240,
		.thb	= 70,
		.thf	= 18,
		.tvb	= 13,
		.tvf	= 10,
	},
	{
		.name	= "HX8238",
		.detect	= hx8238_detect,
		.init	= hx8238_init,
		.xres	= 320,
		.yres	= 240,
		.thb	= 68,
		.thf	= 20,
		.tvb	= 18,
		.tvf	= 4,
	},
};

#define MODULE_DEFAULT	0

/*
 * DPC (display controller) PVD (Primary Video Data) pins
 *   When configured for Alternate Function 1, GPIOA[0-7] and GPIOB[16-31]
 *   are the primary video data pins.
 */

#define NUM_PVD_PINS 24

static const char pvd_pins[NUM_PVD_PINS] = {
	GPIO_PIN0,   GPIO_PIN1,   GPIO_PIN2,   GPIO_PIN3,   GPIO_PIN4, 
	GPIO_PIN5,   GPIO_PIN6,   GPIO_PIN7,   GPIO_PIN16,  GPIO_PIN17,  
	GPIO_PIN18,  GPIO_PIN19,  GPIO_PIN20,  GPIO_PIN21,  GPIO_PIN22,  
	GPIO_PIN23,  GPIO_PIN24,  GPIO_PIN25,  GPIO_PIN26,  GPIO_PIN27,  
	GPIO_PIN28,  GPIO_PIN29,  GPIO_PIN30,  GPIO_PIN31
};
static const char pvd_ports[NUM_PVD_PINS] = {
	GPIO_PORT_A, GPIO_PORT_A, GPIO_PORT_A, GPIO_PORT_A, GPIO_PORT_A,
	GPIO_PORT_A, GPIO_PORT_A, GPIO_PORT_A, GPIO_PORT_B, GPIO_PORT_B,
	GPIO_PORT_B, GPIO_PORT_B, GPIO_PORT_B, GPIO_PORT_B, GPIO_PORT_B,
	GPIO_PORT_B, GPIO_PORT_B, GPIO_PORT_B, GPIO_PORT_B, GPIO_PORT_B,
	GPIO_PORT_B, GPIO_PORT_B, GPIO_PORT_B, GPIO_PORT_B
};

struct display_module *display_setup(void)
{
	int i;

	/* Try to detect one of the supported LCD modules, if we detect
	 * one, initialize it and the DPC accordingly. */
	for (i = 0; i < sizeof(modules)/sizeof(modules[0]); i++) {
		if (modules[i].detect()) {
			db_puts("LCD: ");
			db_puts(modules[i].name);
			db_puts("\n");
			
			modules[i].init();
			dpc_init(&modules[i]);
			return &modules[i];
		}
	}

	/* No module was detected.  We'll fire up the DPC without
	 * specific settings */
	db_puts("LCD: unknown!\n");
	dpc_init(&modules[MODULE_DEFAULT]);

	return 0;
}

void display_init(void)
{
	int i;
	global_var * gptr = get_global();

	/* set up PVD (LCD interface) IO pins */
	for(i = 0; i < NUM_PVD_PINS; i++)
		gpio_configure_pin(pvd_ports[i], pvd_pins[i],
				GPIO_ALT1, 1, 0, 0);

	/* turn on the Display Controller */
	dpc_enable();

	/* bring up the Multi-Layer Controller */
	if (gptr->display_info)
		mlc_init(gptr->display_info->xres, gptr->display_info->yres);
	else
		mlc_init(320, 240);
}

/* 
 * turn on the backlight 
 */
void display_backlight(void)
{
	global_var * gptr = get_global();

	if (gptr->use_lfp100) {
		db_puts("LFP100 backlight\n");
		lfp100_backlight_enable();
		return;
	}

	// Assume PWM 
	switch(gptr->board_id) {
	case LF1000_BOARD_DEV:
	case LF1000_BOARD_DIDJ:
	case LF1000_BOARD_DIDJ_09:
	case LF1000_BOARD_ACORN:
	case LF1000_BOARD_EMERALD_POP:
	case LF1000_BOARD_EMERALD_NOTV_NOCAP:
	case LF1000_BOARD_EMERALD_TV_NOCAP:
	case LF1000_BOARD_EMERALD_NOTV_CAP:
	case LF1000_BOARD_EMERALD_SAMSUNG:
	case LF1000_BOARD_K2:
		gpio_configure_pin(GPIO_PORT_A, GPIO_PIN30, GPIO_GPIOFN, 1, 0, 1);
		pwm_init(1);
		break;
	case LF1000_BOARD_MADRID:
	case LF1000_BOARD_MADRID_POP:
		// This guy better use lfp100 or PWM soon!
		gpio_configure_pin(GPIO_PORT_C, GPIO_PIN7, GPIO_GPIOFN, 1, 0, 1);
		pwm_init(2);
		break;
	default:
		db_puts("unknown board type\n");
		break;
	}
	return;
}

#if 1	// 5apr11

/* returns 1 if the expected lcd module is detected; 
 * otherwise returns 0.
 */
struct display_module * madrid_display_setup(struct madrid_display_module *pdm)
{
	if (ili6480g2_detect()) {
		ili6480g2_init();
		
		pdm->xres	= 480;
		pdm->yres	= 272;
		pdm->thb	= 40;
		pdm->thf	= 5;
		pdm->tvb	= 8;
		pdm->tvf	= 8;
		pdm->name	= "ILI6480G2";

		dpc_init((struct display_module *)pdm);
		db_puts("LCD: ILI6480G2\n");
		return (struct display_module *)pdm;
	}
	pdm->xres	= 320;
	pdm->yres	= 240;
	pdm->thb	= 36;
	pdm->thf	= 50;
	pdm->tvb	= 17;
	pdm->tvf	= 4;
	dpc_init((struct display_module *)pdm);
	db_puts("LCD: unknown!\n");

	return 0;
}

#endif	// 5apr11

