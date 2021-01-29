/* pwm.c -- Basic PWM driver for backlight control.
 * 
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <board.h>
#include <base.h>
#include <gpio.h>

/* registers, as offsets from PWM_BASE */
#define PWM01PRES		0x00
#define PWM2PRES		0x10
#define PWM0DUTY		0x02
#define PWM1DUTY		0x04
#define PWM2DUTY		0x12
#define PWM0PERIOD		0x06
#define PWM1PERIOD		0x08
#define PWM2PERIOD		0x16
#define PWMCLKENB		0x40
#define PWMCLKGEN		0x44

/* PWM PRESCALER0/1 REGISTER (PWM01PRES) */
#define PWM1POL			15
#define PWM1PRESCALE		8
#define PWM0POL			7
#define PWM0PRESCALE		0
#define PWM2POL			7
#define PWM2PRESCALE		0

/* PWM CLOCK ENABLE REGISTER (PWMCLKENB) */
#define PWMPCLKMODE		3
#define PWMCLKGENENB		2

/* PWM CLOCK GENERATE REGISTER (PWMCLKGEN) */
#define PWMCLKDIV		4
#define PWMCLKSRCSEL		1

#define PWM16(x)	REG16(PWM_BASE+x)
#define PWM32(x)	REG32(PWM_BASE+x)

/* Pin definitions */
#define PWM_IO_PORT0	GPIO_PORT_A
#define PWM_IO_PIN0	30
#define PWM_IO_PORT1	GPIO_PORT_A
#define PWM_IO_PIN1	31
#define PWM_IO_PORT2	GPIO_PORT_C
#define PWM_IO_PIN2	7

#define PWM_SRC		1	/* PLL1 */
#define PWM_DIV		14	/* divide 147/15 for 9.8MHz */
#define PWM_INITIAL	318	/* matches WLED_INITIAL (~16 * 19) */
#define PWM_PERIOD	512

static u16 prescale_register[] = {PWM01PRES, PWM01PRES, PWM2PRES};
static u16 duty_register[] = {PWM0DUTY, PWM1DUTY, PWM2DUTY};
static u16 period_register[] = {PWM0PERIOD, PWM1PERIOD, PWM2PERIOD};

static u16 polarity_loc[] = {PWM0POL, PWM1POL, PWM2POL};
static u16 prescale_loc[] = {PWM0PRESCALE, PWM1PRESCALE, PWM2PRESCALE};

static u16 io_port[] = {PWM_IO_PORT0, PWM_IO_PORT1, PWM_IO_PORT2};
static u16 io_pin[] = {PWM_IO_PIN0, PWM_IO_PIN1, PWM_IO_PIN2};

void pwm_init(int channel)
{
	u16 tmp;

	/* configure clock */
	PWM16(PWMCLKGEN) = (PWM_DIV<<PWMCLKDIV|PWM_SRC<<PWMCLKSRCSEL);
	PWM32(PWMCLKENB) |= (1<<PWMPCLKMODE)|(1<<PWMCLKGENENB);

	/* configure prescaler */
	tmp = PWM16(prescale_register[channel]);
	/* set the prescaler to 1 (no prescale) */
	tmp &= ~(0x3F<<prescale_loc[channel]);
	tmp |= (1<<prescale_loc[channel]);
	/* set the polarity to 'bypass' */
	BIT_SET(tmp, polarity_loc[channel]);
	PWM16(prescale_register[channel]) = tmp;

	/* set PWM to second brightest level */
	PWM16(period_register[channel]) = PWM_PERIOD;
	PWM16(duty_register[channel]) = PWM_INITIAL;	/* second brightest */
	
	/* turn on PWM pin */
	gpio_configure_pin(io_port[channel], io_pin[channel], GPIO_ALT1, 1, 0, 0);
}
