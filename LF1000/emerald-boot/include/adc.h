/* adc.h  -- analog to digital converter
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ADC_H__
#define __ADC_H__

/* model ADC as a line:
 * milliVolts = ((ADC_SLOPE * 256) * READING) / 256 + ADC_CONSTANT */

#define ADC_SLOPE_256	2012	/* ADC slope * 256  */
#define ADC_CONSTANT	0	/* ADC constant */

#define ADC_TO_MV(r)    (((ADC_SLOPE_256*r)/256) + ADC_CONSTANT)

void adc_init(void);
u16 adc_get_reading(u8 channel);

#endif /* __ADC_H__ */
