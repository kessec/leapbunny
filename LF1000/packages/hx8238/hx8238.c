/* hx8238.c - Basic test code to read and write HX8238 registers.  This
 *            application uses the Leapfrog GPIO API and bit-bangs the bus
 *            rather than actually using the SPI controller.  It's meant for
 *            quick experimentation with HX8238 settings. 
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com> */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <linux/lf1000/gpio_ioctl.h>

#define SPI_PORT	1
#define SCK_PIN		13
#define MOSI_PIN	15
#define MISO_PIN	14
#define SS_PIN		12

#define clk(v)	pin(SPI_PORT, SCK_PIN, v)
#define dat(v)	pin(SPI_PORT, MOSI_PIN, v)
#define enb(v)	pin(SPI_PORT, SS_PIN, v)

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

int tx(unsigned int val)
{
	unsigned int in;

	clk(0);
	dat(!!val);
	usleep(1);
	get(SPI_PORT, MISO_PIN, &in);
	clk(1);
	
	return in;
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
	if (setup_pin(SPI_PORT, SCK_PIN, 1))
		return 1;
	if (setup_pin(SPI_PORT, MOSI_PIN, 1))
		return 1;
	if (setup_pin(SPI_PORT, MISO_PIN, 0))
		return 1;
	if (setup_pin(SPI_PORT, SS_PIN, 1))
		return 1;
	
	return 0;
}

unsigned int spi_transfer24(unsigned int value)
{
	unsigned int in;
	unsigned int res = 0;
	int i;

	clk(0);
	enb(0);
	usleep(1);

	for (i = 0; i <= 23; i++) {
		in = tx(value & (1<<(23-i)));
		res |= (in<<(23-i));
	}

	enb(1);
	return res;
}

int main(int argc, char **argv)
{
	unsigned int reg, data;

	if (argc < 3) {
		printf("usage: %s <device> <register> [data]\n", argv[0]);
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

	if (sscanf(argv[2], "%X", &reg) != 1) {
		printf("Error: invalid register\n");
		close(dev);
		return 3;
	}

	/* start with SS deasserted */
	enb(1);

	if (argc < 4) {
		spi_transfer24(0x70<<16 | reg);
		data = spi_transfer24(0x73<<16);
		printf("0x%04X\n", data);
	} else {
		if (sscanf(argv[3], "%X", &data) != 1) {
			printf("Error: invalid data\n");
			close(dev);
			return 4;
		}

		spi_transfer24(0x70<<16 | reg);
		spi_transfer24(0x72<<16 | data);
	}

	close(dev);
	return 0;
}
