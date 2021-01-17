/* adc.h  -- analog to digital converter API
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ADC_H
#define ADC_H

void adc_init(void);
u16 adc_get_reading(u8 channel);

#endif /* ADC_H */
