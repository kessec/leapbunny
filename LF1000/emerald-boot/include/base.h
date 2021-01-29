/* base.h - SoC peripherals memory map
 * 
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BASE_H__
#define __BASE_H__

#define IC_BASE		0xC0000800
#define ADC_BASE	0xC0005000
#define SPI0_BASE	0xC0007800
#define SDHC0_BASE	0xC0009800
#define SDHC1_BASE	0xC000C800
#define TIMER0_BASE	0xC0001800
#define GPIO_BASE	0xC000A000
#define PWM_BASE	0xC000C000
#define I2C_BASE	0xC000E000
#define CLKPWR_BASE	0xC000F000
#define RTC_BASE	0xC000F080
#define GPIOCURRENT_BASE 0xC000F100
#define MCU_S_BASE	0xC0015800
#define UART0_BASE	0xC0016000
#define UDC_BASE	0xC0018000
#define ALIVE_BASE	0xC0019000
#define DPC_BASE	0xC0003000
#define MLC_BASE	0xC0004000

#define NAND_BASE_LOW	0x2C000000
#define NAND_BASE_HIGH	0xAC000000

#endif /* __BASE_H__ */
