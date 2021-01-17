/* threewire.c - Bit-bang "three wire serial" bus as per the NT39016 data
 *               sheet via the Leapfrog GPIO framework.  This enables us to
 *               read and write NT39016 registers.
 *
 * Copyright 2007-2009 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <linux/lf1000/gpio_ioctl.h>

#define SCK_PORT	1
#define SCK_PIN		13

#define SPD_PORT	1
#define SPD_PIN		14

#define SPE_PORT	1
#define SPE_PIN		12

#define clk(v)	pin(SCK_PORT, SCK_PIN, v)
#define dat(v)	pin(SPD_PORT, SPD_PIN, v)
#define enb(v)	pin(SPE_PORT, SPE_PIN, v)

#define DATA_BITS 7

int dev = -1;

int pin(int port, int pin, unsigned int val)
{
	union gpio_cmd c;

	c.outvalue.port = port;
	c.outvalue.pin = pin;
	c.outvalue.value = val;
	if (ioctl(dev, GPIO_IOCSOUTVAL, &c) < 0) {
		perror("setting output value\n");
		return 1;
	}

	return 0;
}

int get(int port, int pin, unsigned int *val)
{
	union gpio_cmd c;

	c.invalue.port = port;
	c.invalue.pin = pin;
	if (ioctl(dev, GPIO_IOCXINVAL, &c) < 0)
		return 1;

	*val = c.invalue.value;
	return 0;
}

int rx(unsigned int *val)
{
	if (clk(0))
		return 1;
	if (get(SPD_PORT, SPD_PIN, val))
		return 1;
	if (clk(1))
		return 1;

	return 0;
}

int tx(unsigned int val)
{
	if (clk(0))
		return 1;
	if (dat(!!val))
		return 1;
	if (clk(1))
		return 1;
	
	return 0;
}

int setup_pin(int port, int pin, int outen)
{
	union gpio_cmd c;

	c.func.port = port;
	c.func.pin = pin;
	c.func.func = 0;
	if (ioctl(dev, GPIO_IOCSFUNC, &c) < 0) {
		perror("setting pin function\n");
		return 1;
	}

	c.outenb.port = port;
	c.outenb.pin = pin;
	c.outenb.value = outen;
	if (ioctl(dev, GPIO_IOCSOUTENB, &c) < 0) {
		perror("setting output enable\n");
		return 1;
	}

	return 0;
}

int setup_pins(void)
{
	if (setup_pin(SCK_PORT, SCK_PIN, 1))
		return 1;
	if (setup_pin(SPD_PORT, SPD_PIN, 1))
		return 1;
	if (setup_pin(SPE_PORT, SPE_PIN, 1))
		return 1;
	
	return 0;
}

int write_register(unsigned int reg, unsigned int value)
{
	int i;

	if (setup_pin(SPD_PORT, SPD_PIN, 1)) {
		enb(1);
		return 1;
	}

	clk(0);
	enb(0);
	usleep(1);

	/* write out the 6-bit address */
	for (i = 0; i <= 5; i++) {
		if (tx((reg & (1<<(5-i))))) {
			enb(1);
			return 1;
		}
	}

	/* 1 for write mode */
	tx(1);

	/* 1 for Hi-Z mode (ignored) */
	tx(1);

	/* write out the 8-bit data */
	
	for (i = 0; i <= DATA_BITS; i++) {
		if (tx(value & (1<<(DATA_BITS-i)))) {
			enb(1);
			return 1;
		}
	}

	enb(1);
	return 0;
}

int read_register(unsigned int reg)
{
	int i;
	unsigned int val;
	int res = 0;

	if (setup_pin(SPD_PORT, SPD_PIN, 1)) {
		enb(1);
		return -1;
	}

	clk(0);
	enb(0);
	usleep(1);

	/* write out the 6-bit address */
	for (i = 0; i <= 5; i++) {
		if (tx(reg & (1<<(5-i)))) {
			enb(1);
			return -1;
		}
	}

	/* 0 for read mode */
	tx(0);

	/* 1 for Hi-Z mode */
	tx(1);

	/* clock in the 8-bit data */
	
	if (setup_pin(SPD_PORT, SPD_PIN, 0)) {
		enb(1);
		return -1;
	}

	for (i = 0; i <= DATA_BITS; i++) {
		if (rx(&val)) {
			enb(1);
			return -1;
		}
		res |= (val<<(DATA_BITS-i));
	}

	enb(1);
	return res;
}

int main(int argc, char **argv)
{
	unsigned int reg, value;
	int res;

	if (argc < 4) {
		printf("usage: %s <device> <r|w> <reg> [data]\n", argv[0]);
		return 0;
	}

	dev = open(argv[1], O_RDWR);
	if (dev < 0) {
		perror("opening device\n");
		return 1;
	}

	if (setup_pins()) {
		printf("Error: unable to set up pins\n");
		close(dev);
		return 2;
	}

	if (sscanf(argv[3], "%X", &reg) != 1) {
		printf("Error: invalid register\n");
		close(dev);
		return 3;
	}

	/* start with SPE deasserted */
	enb(1);

	if (!strcmp(argv[2], "r")) {
		res = read_register(reg);
		if (res >= 0)
			printf("reg 0x%02X = 0x%02X\n", reg, res);
		else
			printf("Error: unable to read register\n");
	} else if (!strcmp(argv[2], "w")) {
		if (argc < 5) {
			printf("Error: supply a value to write\n");
			close(dev);
			return 3;
		}

		if (sscanf(argv[4], "%X", &value) == 1) {
			if (write_register(reg, value))
				printf("Error: unable to write register\n");
		}
		else
			printf("Error: invalid value\n");
	}

	close(dev);
	return 0;
}
