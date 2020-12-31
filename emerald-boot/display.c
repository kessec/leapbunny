/* display.c -- Basic display driver.
 * 
 * Copyright 2007-2009 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "include/autoconf.h"
#include "include/mach-types.h" /* for machine info */
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/gpio.h>

#include "include/gpio.h"
#include "include/dpc.h"
#include "include/mlc.h"
#include "include/pwm.h"
#include "include/display.h"
#include "include/debug.h"

/* Table of supported display modules and their DPC settings. */
struct display_module modules[] = {
	{
		.name	= "NT39016",
		.detect	= nt39016_detect,
		.init	= nt39016_init,
		.thb	= 70,
		.thf	= 18,
		.tvb	= 13,
		.tvf	= 10,
	},
	{
		.name	= "ILI9322",
		.detect	= ili9322_detect,
		.init	= ili9322_init,
		.thb	= 36,
		.thf	= 50,
		.tvb	= 17,
		.tvf	= 4,
	},
	{
		.name	= "HX8238",
		.detect	= hx8238_detect,
		.init	= hx8238_init,
		.thb	= 68,
		.thf	= 20,
		.tvb	= 18,
		.tvf	= 4,
	},
};

/*
 * DPC (display controller) PVD (Primary Video Data) pins
 *   When configured for Alternate Function 1, GPIOA[0-7] and GPIOB[16-31]
 *   are the primary video data pins.
 */

#define NUM_PVD_PINS 24

static char pvd_pins[NUM_PVD_PINS] = {
	GPIO_PIN0,   GPIO_PIN1,   GPIO_PIN2,   GPIO_PIN3,   GPIO_PIN4, 
    GPIO_PIN5,   GPIO_PIN6,   GPIO_PIN7,   GPIO_PIN16,  GPIO_PIN17,  
    GPIO_PIN18,  GPIO_PIN19,  GPIO_PIN20,  GPIO_PIN21,  GPIO_PIN22,  
    GPIO_PIN23,  GPIO_PIN24,  GPIO_PIN25,  GPIO_PIN26,  GPIO_PIN27,  
    GPIO_PIN28,  GPIO_PIN29,  GPIO_PIN30,  GPIO_PIN31
};
static char pvd_ports[NUM_PVD_PINS] = {
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
			dpc_init(modules[i].thb, modules[i].thf,
				 modules[i].tvb, modules[i].tvf);
			return &modules[i];
		}
	}

	/* No module was detected.  We'll fire up the DPC without
	 * specific settings */
	db_puts("LCD: unknown!\n");
	dpc_init(36, 50, 17, 4); /* use ILI settings */

	return NULL;
}

void display_init(void)
{
	int i;

	/* set up PVD (LCD interface) IO pins */
	for(i = 0; i < NUM_PVD_PINS; i++)
		gpio_configure_pin(pvd_ports[i], pvd_pins[i],
				GPIO_ALT1, 1, 0, 0);

	/* turn on the Display Controller */
	dpc_enable();

	/* bring up the Multi-Layer Controller */
	mlc_init();
}

/* 
 * turn on the backlight 
 */
void display_backlight(int board_id)
{
	gpio_configure_pin(GPIO_PORT_A, 30, GPIO_GPIOFN, 1, 0, 1);
	pwm_init(board_id);
}
