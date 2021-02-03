/* adc.c -- Basic ADC functionality for battery monitoring
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 * Scott Esters <sesters@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <base.h>

/*
 * ADC Registers (offsets from ADC_BASE) 
 */
#define ADCCON                  0x00
#define ADCDAT                  0x04
#define ADCCLKENB               0x40

/* ADC control register (ADCCON) */
#define APEN                    14
#define APSV                    6
#define ASEL                    3
#define ADEN                    0

#define ADC32(r)	REG32(ADC_BASE+r)
#define ADC16(r)	REG16(ADC_BASE+r)

void adc_init(void)
{
	u16 tmp;

	/* enable PCLK, its only allowed mode is 'always' */
	ADC32(ADCCLKENB) = (u32)(1<<3);

	/* power is off, out of standby, set prescale */
	ADC16(ADCCON) = 0;
	tmp = (147<<APSV);
	ADC16(ADCCON) = tmp;
	/* power on the ADC */
	tmp |= (1<<APEN);
	ADC16(ADCCON) = tmp;
}

u16 adc_get_reading(u8 channel)
{
	u16 tmp;

	/* set channel, request conversion */
	tmp = ADC16(ADCCON) & ~(0x7<<ASEL);
	tmp |= ((1<<ADEN)|(channel<<ASEL));
	ADC16(ADCCON) = tmp;
	
	/* wait for conversion to finish */
	while(IS_SET(ADC16(ADCCON), ADEN));

	/* read the result */
	return (0x3FF & ADC16(ADCDAT));
}
