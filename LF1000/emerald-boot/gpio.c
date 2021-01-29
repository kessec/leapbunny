/* gpio.c -- GPIO functions.
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
#include <base.h>
#include <gpio.h>
#include <gpio_hal.h>

#define GPIO32(x)		REG32(GPIO_BASE+x)
#define ALIVE32(x)		REG32(ALIVE_BASE+x)
#define GPIOCURRENT32(x)	REG32(GPIOCURRENT_BASE+x)

void gpio_set_fn(enum gpio_port port, enum gpio_pin pin, enum gpio_function f)
{
	u32 reg, tmp;

	if (port == GPIO_PORT_ALV)
		return;
	
	reg = GPIOAALTFN0 + port*0x40;
	
	if (pin >= 16) {
		reg += 4;
		pin -= 16;
	}
	
	pin *= 2;
	tmp = GPIO32(reg);
	tmp &= ~(3<<pin);
	tmp |= (f<<pin);
	GPIO32(reg) = tmp;
}

void gpio_set_out_en(enum gpio_port port, enum gpio_pin pin, unsigned char en)
{
	u32 reg;

	if (port == GPIO_PORT_ALV)
		return;

	reg = GPIOAOUTENB + port*0x40;
	if (en)
		BIT_SET(GPIO32(reg), pin);
	else
		BIT_CLR(GPIO32(reg), pin);
}

/* TODO: add Alive port support, if needed */
int gpio_get_val(enum gpio_port port, enum gpio_pin pin)
{
	return ((GPIO32(GPIOAPAD + port*0x40) >> pin) & 0x1);
}

void gpio_set_val(enum gpio_port port, enum gpio_pin pin, u8 en)
{
	u32 reg, set_reg, clr_reg;

	if (port == GPIO_PORT_ALV) {
		/* enable writing to Alive registers */
		BIT_SET(ALIVE32(ALIVEPWRGATEREG), NPOWERGATING);

		/* we're operating an S/R flip-flop */
		if (en) {
			set_reg = ALIVEGPIOSETREG;
			clr_reg = ALIVEGPIORSTREG;
		}
		else {
			set_reg = ALIVEGPIORSTREG;
			clr_reg = ALIVEGPIOSETREG;
		}

		BIT_CLR(ALIVE32(clr_reg), pin);
		BIT_SET(ALIVE32(set_reg), pin);
		
		/* disable writing to Alive registers */
		BIT_CLR(ALIVE32(ALIVEPWRGATEREG), NPOWERGATING);

		return;
	}

	reg = GPIOAOUT + port*0x40;
	if (en)
		BIT_SET(GPIO32(reg), pin);
	else
		BIT_CLR(GPIO32(reg), pin);
}

void gpio_set_pu(enum gpio_port port, enum gpio_pin pin, unsigned char en)
{
	u32 reg;

	if (port == GPIO_PORT_ALV)
		return;

	reg = GPIOAPUENB + port*0x40;
	if (en)
		BIT_SET(GPIO32(reg), pin);
	else
		BIT_CLR(GPIO32(reg), pin);
}

void gpio_configure_pin(enum gpio_port port, enum gpio_pin pin, 
		enum gpio_function f, u8 out_en, u8 pu_en, u8 val)
{
	gpio_set_fn(port, pin, f);
	gpio_set_out_en(port, pin, out_en);
	gpio_set_pu(port, pin, pu_en);
	gpio_set_val(port, pin, val);
}

/* get gpio pin drive current setting */
unsigned long gpio_get_cur(enum gpio_port port, enum gpio_pin pin)
{
	u32 reg;

	if((port == GPIO_PORT_ALV) ||
			((port == GPIO_PORT_C) && (pin > GPIO_PIN19)))
		/* We can't set current for GPIO ALIVE block or for GPIOC pins
		 * above 19 
		 */
		return 0;

	reg = port*8;
	if(pin < 16) {
		reg += GPIOPADSTRENGTHGPIOAL;
	} else {
		reg += GPIOPADSTRENGTHGPIOAH;
		pin -= 16;
	}
	return ((GPIOCURRENT32(reg) >> (pin<<1)) & 0x3);
}

/* set the drive current for the gpio pin */
void gpio_set_cur(enum gpio_port port, enum gpio_pin pin, enum gpio_current cur)
{
	u32 reg;
	unsigned long tmp;

	if( (port == GPIO_PORT_ALV) ||
			((port == GPIO_PORT_C) && (pin > GPIO_PIN19)))
		/* We can't set current for GPIO ALIVE block or for
		 * GPIOC pins above 19 
		 */
		return;

	reg = port*8;
	if(pin < 16) {
		reg += GPIOPADSTRENGTHGPIOAL;
	} else {
		reg += GPIOPADSTRENGTHGPIOAH;
		pin -= 16;
	}

	tmp = GPIOCURRENT32(reg);
	tmp &= ~(0x3 << (pin<<1));
	tmp |= (cur << (pin<<1));
	GPIOCURRENT32(reg) = tmp;

}

u32 gpio_get_scratchpad(void)
{
	return ALIVE32(ALIVESCRATCHREADREG);
}

void gpio_set_scratchpad(u32 val)
{
	/* enable writing to registers */
	BIT_SET(ALIVE32(ALIVEPWRGATEREG), NPOWERGATING);

	/* clear */
	ALIVE32(ALIVESCRATCHRSTREG) = 0;
	ALIVE32(ALIVESCRATCHSETREG) = 0;

	/* set */
	ALIVE32(ALIVESCRATCHRSTREG) = ~val;
	ALIVE32(ALIVESCRATCHSETREG) = val;

	/* latch */
	ALIVE32(ALIVESCRATCHRSTREG) = 0;
	ALIVE32(ALIVESCRATCHSETREG) = 0;

	/* disable writing to registers */
	BIT_CLR(ALIVE32(ALIVEPWRGATEREG), NPOWERGATING);

}
