/* led.h -- LED test code
 * 
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LED_CONFIG_H__
#define __LED_CONFIG_H__

#include <board.h>

#if 1 || defined(BOARD_STRESS_TEST)

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

// put in for timing events - kward
#if 1	// Added this disabled branch in order to allow booting from ATAP nand.
#define FW_DEBUG_0_OFF 
#define FW_DEBUG_1_OFF 
#define FW_DEBUG_2_OFF 
#define FW_DEBUG_3_OFF 

#define FW_DEBUG_0_ON 
#define FW_DEBUG_1_ON 
#define FW_DEBUG_2_ON 
#define FW_DEBUG_3_ON 

#else	// enable this branch if you want to use the gpio lines for timing
		// signals.

#define FW_DEBUG_0_OFF *((volatile unsigned int*)0xc000a080) &= ~(0x00000002)
#define FW_DEBUG_1_OFF *((volatile unsigned int*)0xc000a080) &= ~(0x00000100)
#define FW_DEBUG_2_OFF *((volatile unsigned int*)0xc000a080) &= ~(0x00000200)
#define FW_DEBUG_3_OFF *((volatile unsigned int*)0xc000a080) &= ~(0x00000800)

#define FW_DEBUG_0_ON *((volatile unsigned int*)0xc000a080) |= 0x00000002
#define FW_DEBUG_1_ON *((volatile unsigned int*)0xc000a080) |= 0x00000100
#define FW_DEBUG_2_ON *((volatile unsigned int*)0xc000a080) |= 0x00000200
#define FW_DEBUG_3_ON *((volatile unsigned int*)0xc000a080) |= 0x00000800

/* Configure GPIOC_1, _8, _9, _11 and GPIOB_1 for wired Madrid cart conn */
static void fw_debug_led_config() {
        // config GPIOC_1 _8 _9 11 for their GPIO function
        //   GPIOC Alternate Function Select Register is at 0xc000a0a0
        *((volatile unsigned int*)0xc000a0a0) &= ~(0x00cf000c);

        // config GPIOC_1 _8 _9 11 to be output pins
        //   GPIOC Output Enable Register is at 0xc000a084
        *((volatile unsigned int*)0xc000a084) |= 0x00000b02;
}

#endif

#endif /* __STRESS_TEST_H__ */
