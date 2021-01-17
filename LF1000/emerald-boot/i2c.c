/* i2c.c - Basic polling I2C driver
 *
 * Copyright 2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdint.h>
#include <mach/common.h>
#include <mach/gpio.h>
#include <mach/i2c.h>
#include "include/autoconf.h"
#include "include/mach-types.h"
#include "include/board.h"

#define I2C_NUM_CHANNELS	2
#define I2C(c, x)		REG32(LF1000_I2C0_BASE+c*0x800+x)

static inline void i2c_start_stop_condition(u8 ch)
{
	I2C(ch, IRQ_PEND) |= (1<<OP_HOLD);
}

static inline int i2c_got_ack(u8 ch)
{
	int i = 0;

	while (!I2C(ch, ICSR) & (1<<ACK_STATUS) && ++i < 10000);

	return i >= 10000 ? 0 : 1;
}

static int i2c_wait_for_irq(u8 ch)
{
	int i = 0;

	while (!(I2C(ch, IRQ_PEND) & (1<<PEND)) && ++i < 10000);
	
	if (i >= 10000)
		return 1;

	I2C(ch, IRQ_PEND) |= (1<<PEND);

	return 0;
}

static int i2c_start(u8 ch)
{
	if (ch >= I2C_NUM_CHANNELS)
		return 1;

	I2C(ch, ICCR) = 0;
	I2C(ch, BURST_CTRL) = 0;

	I2C(ch, ICCR) = (1<<CLK_SRC)|((8-1)<<CLK_SCALER);
	I2C(ch, QCNT_MAX) = (1<<CNT_MAX);
	I2C(ch, I2C_CLKENB) |= (1<<3);
	I2C(ch, ICSR) = 0x1010;

	i2c_start_stop_condition(ch);

	return 0;
}

static int i2c_send_addr(u8 ch, u8 addr)
{
	u32 tmp;

	I2C(ch, IDSR) = addr;

	if (addr & 0x01) { /* read */
		tmp = I2C(ch, ICSR) & 0x1F0F;
		tmp |= (1<<IRQ_ENB)|(1<<ST_ENB)|(1<<MASTER_SLV)|(1<<ST_BUSY)|
			(1<<TXRX_ENB);
		I2C(ch, ICSR) = tmp;
	} else { /* write */
		tmp = I2C(ch, ICSR) & 0x10F0;
		tmp |= (1<<IRQ_ENB)|(1<<ST_ENB)|(1<<MASTER_SLV)|(1<<ST_BUSY)|
			(1<<TXRX_ENB)|(1<<TX_RX);
		I2C(ch, ICSR) = tmp;
	}
	i2c_start_stop_condition(ch); /* start */

	if (i2c_wait_for_irq(ch))
		return 2;

	if (!i2c_got_ack(ch))
		return 3;

	return 0;
}

int i2c_read(u8 ch, u8 addr, u8 *buf, unsigned int len)
{
	u32 tmp;
	int ret = 0;
	unsigned int i;

	if (ch >= I2C_NUM_CHANNELS)
		return 1;

	i2c_start(ch);

	ret = i2c_send_addr(ch, addr | 0x01);
	if (ret)
		goto out_read;

	/* receive the data */

	for (i = 0; i < len; i++) {
		tmp = I2C(ch, IDSR);
		buf[i] = (u8)tmp;
		i2c_start_stop_condition(ch);
		if (i2c_wait_for_irq(ch)) {
			ret = 4;
			goto out_read;
		}
	}

	/* done */
	tmp = I2C(ch, ICSR) & 0x1F0F;
	tmp |= (1<<ST_ENB)|(1<<MASTER_SLV)|(1<<TXRX_ENB);
	I2C(ch, ICSR) = tmp;
	i2c_start_stop_condition(ch);
out_read:
	I2C(ch, ICSR) = 0;
	I2C(ch, I2C_CLKENB) &= ~(1<<3);

	return ret;
}

int i2c_write(u8 ch, u8 addr, u8 *buf, unsigned int len)
{
	u32 tmp;
	int ret = 0;
	unsigned int i;

	if (ch >= I2C_NUM_CHANNELS)
		return 1;

	i2c_start(ch);

	ret = i2c_send_addr(ch, addr);
	if (ret)
		goto out_write;

	/* send the data */

	for (i = 0; i < len; i++) {
		I2C(ch, IDSR) = buf[i];
		i2c_start_stop_condition(ch);
		if (i2c_wait_for_irq(ch)) {
			ret = 4;
			goto out_write;
		}
		if (!i2c_got_ack(ch)) {
			ret = 5;
			goto out_write;
		}
	}

	/* done */
out_write:
	tmp = I2C(ch, ICSR) & 0x1F0F;
	tmp |= (1<<ST_ENB)|(1<<MASTER_SLV)|(1<<TX_RX)|(1<<TXRX_ENB);
	I2C(ch, ICSR) = tmp;
	i2c_start_stop_condition(ch);
	I2C(ch, ICSR) = 0;
	I2C(ch, I2C_CLKENB) &= ~(1<<3);

	return ret;
}

int i2c_init(u8 ch)
{
	if (ch >= I2C_NUM_CHANNELS)
		return 1;
	
	gpio_configure_pin(GPIO_PORT_A, 26+2*ch, GPIO_ALT1, 1, 0, 0); /* SCL */
	gpio_configure_pin(GPIO_PORT_A, 27+2*ch, GPIO_ALT1, 1, 0, 0); /* SDA */
	
	return 0;
}
