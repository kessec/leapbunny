/* led.h -- LED test code
 * 
 * Copyright 2010 LeapFrog Enterprises Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LED_CONFIG_H__
#define __LED_CONFIG_H__

#ifdef CONFIG_LF1000_STRESS_TEST

#define led_config()
#define LED1_ON
#define LED2_ON
#define LED1_OFF
#define LED2_OFF

#else

#define LED1_ON *((volatile unsigned int*)0xc000a040) &= ~(0x00000001)
#define LED2_ON *((volatile unsigned int*)0xc000a040) &= ~(0x00000002)

#define LED1_OFF *((volatile unsigned int*)0xc000a040) |= 0x00000001
#define LED2_OFF *((volatile unsigned int*)0xc000a040) |= 0x00000002

/* Configure GPIOB_0 and GPIOB_1 for the cart LEDs */
static void led_config() {
        // set those bit on alt_0 (four bits)
        *((volatile unsigned int*)0xc000a060) &= ~(0x0000000f);

        // set for output
        *((volatile unsigned int*)0xc000a044) |= 0x00000003;
}

#endif

#endif /* __STRESS_TEST_H__ */
