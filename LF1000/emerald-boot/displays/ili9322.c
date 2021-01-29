/* ili9322.c - Detect and initialize the ILI LCD driver.
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdint.h>
#include <common.h>
#include <gpio.h>
#include <board.h>
#include <string.h>
#include <debug.h>
#include <spi.h>

#define ILI_CHIP_ID	0x96
#define ILI_GET(x)	((x | 0x8000) & 0xFFFF)
#define ILI_SET(x)	((x & ~(0x8000)) & 0xFFFF)

#define ILI_CMD_CHIPID      0x0000
#define ILI_CMD_AMPLITUDE   0x0100
#define ILI_CMD_HIGHVOLTAGE 0x0200
#define ILI_CMD_VIEWANGLE   0x0300
#define ILI_CMD_DISPLAY     0x0B00
#define ILI_CMD_CONTRAST    0x0E00
#define ILI_CMD_BRIGHTNESS  0x0F00
#define ILI_CMD_GAMMA1      0x1000
#define ILI_CMD_GAMMA2      0x1100
#define ILI_CMD_GAMMA3      0x1200
#define ILI_CMD_GAMMA4      0x1300
#define ILI_CMD_GAMMA5      0x1400
#define ILI_CMD_GAMMA6      0x1500
#define ILI_CMD_GAMMA7      0x1600
#define ILI_CMD_GAMMA8      0X1700

static u16 ili_reg(u16 op)
{
	u16 res;

	spi_cs(0);
	res = spi_write16(op);
	spi_cs(1);

	return res;
}

int ili9322_detect(void)
{
	u16 res;

	spi_init();

	res = ili_reg(ILI_GET(ILI_CMD_CHIPID));
       	if ((res & 0xFF) == ILI_CHIP_ID)
		return 1;

	return 0;
}

/* Write the recommended settings to the ILI9322 driver. */
void ili9322_init(void)
{
	/* Power Control : Normal display+HVDE Mode+Line Inversion */
	ili_reg(ILI_SET((ILI_CMD_DISPLAY | 0x05)));
	/* VCOM High Voltage : VREG1OUT x 0.87*/
	ili_reg(ILI_SET((ILI_CMD_HIGHVOLTAGE | 0x32)));
	/* VCOM AC Voltage : VREG1OUT x 1.06*/
	ili_reg(ILI_SET((ILI_CMD_AMPLITUDE | 0x12)));
	/* Gamma1 : Gamma Curve*/
	ili_reg(ILI_SET((ILI_CMD_GAMMA1 | 0xA7)));
	/* Gamma2 : */
	ili_reg(ILI_SET((ILI_CMD_GAMMA2 | 0x57)));
	/* Gamma3 : */
	ili_reg(ILI_SET((ILI_CMD_GAMMA3 | 0x73)));
	/* Gamma4 : */
	ili_reg(ILI_SET((ILI_CMD_GAMMA4 | 0x72)));
	/* Gamma5 : */
	ili_reg(ILI_SET((ILI_CMD_GAMMA5 | 0x73)));
	/* Gamma6 : */
	ili_reg(ILI_SET((ILI_CMD_GAMMA6 | 0x55)));
	/* Gamma7 : */
	ili_reg(ILI_SET((ILI_CMD_GAMMA7 | 0x17)));
	/* Gamma8 : */
	ili_reg(ILI_SET((ILI_CMD_GAMMA8 | 0x62)));
}
