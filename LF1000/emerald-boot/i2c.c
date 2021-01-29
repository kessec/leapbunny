/* i2c.c - Basic polling I2C driver
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
#include <gpio.h>
#include <debug.h>

#define I2C_NUM_CHANNELS	2
#define I2C_BASE		0xC000E000
#define I2C(c, x)		REG32(I2C_BASE+c*0x800+x)

/* registers, as offsets from I2C_BASE */
#define ICCR                    0x000
#define ICSR                    0x004
#define IDSR                    0x00C
#define QCNT_MAX                0x010
#define BURST_CTRL              0x014
#define IRQ_PEND                0x024
#define I2C_CLKENB              0x100

/* I2C Control Register (ICCR) */
#define CLK_SRC                         6
#define IRQ_ENB                         5
#define CLK_SCALER                      0

/* I2C Status Register (ICSR) */
#define ST_ENB                          12
#define MASTER_SLV                      7
#define TX_RX                           6
#define ST_BUSY                         5
#define TXRX_ENB                        4
#define ACK_STATUS                      0

/* QCNT_MAX */
#define CNT_MAX                         0

/* IRQ Pending (IRQ_PEND) */
#define OP_HOLD                         1
#define PEND                            0

static inline void i2c_start_stop_condition(u8 ch)
{
	I2C(ch, IRQ_PEND) |= (1<<OP_HOLD);
	I2C(ch, IRQ_PEND) |= (1<<PEND);
}

#define MAX_ACK_LOOP	10000
static inline int i2c_got_ack(u8 ch)
{
	int i = 0;

	while ((I2C(ch, ICSR) & (1<<ACK_STATUS)) && ++i < MAX_ACK_LOOP);

	if (i >= MAX_ACK_LOOP) {
		db_puts("i2c_got_ack NOACK ret(0)\n");
	} else {
		db_puts("i2c_got_ack ACK ret(1)\n");
	}

	return i >= MAX_ACK_LOOP ? 0 : 1;
}

#define MAX_IRQ_LOOP 10000
static int i2c_got_irq(u8 ch)
{
	int i = 0;

	while (!(I2C(ch, IRQ_PEND) & (1<<PEND)) && ++i < MAX_ACK_LOOP);
	
	if (i >= MAX_IRQ_LOOP) {
		db_puts("i2c_got_irq ret(0)\n");
		return 0;
	}

	I2C(ch, IRQ_PEND) |= (1<<PEND);

	db_puts("i2c_got_irq ret(1)\n");
	return 1;
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
	db_puts("i2c_start ret(0)\n");

	return 0;
}

static int i2c_send_addr(u8 ch, u8 addr)
{
	u32 tmp;

	I2C(ch, IDSR) = addr;

	db_puts("i2c_send_addr ch="); db_byte(ch);
	db_puts("  addr="); db_byte(addr); db_putchar('\n');

	tmp = I2C(ch, ICSR) & 0x1F0F;

	if (addr & 0x01) { /* read */
		tmp |= (1<<ST_ENB)|(1<<MASTER_SLV)|(1<<ST_BUSY)|(1<<TXRX_ENB);
		db_puts("i2c_send_addr - read\n");
	} else { /* write */
		tmp |= (1<<ST_ENB)|(1<<MASTER_SLV)|(1<<TX_RX)|
			(1<<ST_BUSY)|(1<<TXRX_ENB);
		db_puts("i2c_send_addr - write\n");
	}
	I2C(ch, ICSR) = tmp;
	i2c_start_stop_condition(ch); /* start */

	if (!i2c_got_irq(ch))
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

	addr |= 1;	/* read bit 0 == 1 */
	db_puts("i2c_read: ch=");db_byte(ch);
	db_puts(" addr=");db_byte(addr);
	db_puts(" buf[0]=");db_byte(buf[0]);
	db_puts(" len=");db_int(len); db_putchar('\n');

	if (ch >= I2C_NUM_CHANNELS)
		return 1;

	i2c_start(ch);

	ret = i2c_send_addr(ch, addr);
	if (ret) {
		db_puts("C. i2c_read ret=");db_int(ret);db_putchar('\n');
		goto out_read;
	}

	/* receive the data */

	for (i = 0; i < len; i++) {
		tmp = I2C(ch, IDSR);
		buf[i] = (u8)tmp;
		i2c_start_stop_condition(ch);
		if (!i2c_got_irq(ch)) {
			ret = 4;
			db_puts("i2c_read ret(4)\n");
			goto out_read;
		}
	}

	/* done */
	tmp = I2C(ch, ICSR) & ~(0x1F0F);
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

	addr &= 0xFE;	/* write bit 0 == 0 */

	db_puts("i2c_write: ch=");db_byte(ch);
	db_puts(" addr=");db_byte(addr);
	db_puts(" buf[0]=");db_byte(buf[0]);
	db_puts(" len=");db_int(len); db_putchar('\n');

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
		if (!i2c_got_irq(ch)) {
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
