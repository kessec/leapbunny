/* gpio.h  -- configure and access LF1000 GPIOs
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __GPIO_H__
#define __GPIO_H__

#define GPIO_NUM_PORTS 3
enum gpio_port {
	GPIO_PORT_A	= 0x0,
	GPIO_PORT_B,
	GPIO_PORT_C,
	GPIO_PORT_ALV,
};

/*
 * Pins for Board Configuration resistors.  These will be configured as input
 * pins and read to generate a board ID when gpio_get_board_config() is called.
 */
#define GPIO_CFG_PORT	GPIO_PORT_B
#define GPIO_CFG_LOW	27
#define GPIO_CFG_HIGH	31

/*
 * Pins for Cart Configuration resistors.  These will be configured as input
 * pins and read to generate a cart ID when gpio_get_cart_config() is called.
 */
#define GPIO_CART_CFG_PORT	GPIO_PORT_B
#define GPIO_CART_CFG_LOW	2
#define GPIO_CART_CFG_HIGH	5

/* Each pin can take one of three different functions */
enum gpio_function {
	GPIO_GPIOFN	= 0,
	GPIO_ALT1,
	GPIO_ALT2,
	GPIO_RESERVED,
};

/* Each port has 32 pins */
enum gpio_pin {
	GPIO_PIN0 = 0, GPIO_PIN1, GPIO_PIN2, GPIO_PIN3, GPIO_PIN4, GPIO_PIN5,
	GPIO_PIN6, GPIO_PIN7, GPIO_PIN8, GPIO_PIN9, GPIO_PIN10, GPIO_PIN11,
	GPIO_PIN12, GPIO_PIN13, GPIO_PIN14, GPIO_PIN15, GPIO_PIN16, GPIO_PIN17,
	GPIO_PIN18, GPIO_PIN19, GPIO_PIN20, GPIO_PIN21, GPIO_PIN22, GPIO_PIN23,
	GPIO_PIN24, GPIO_PIN25, GPIO_PIN26, GPIO_PIN27, GPIO_PIN28, GPIO_PIN29,
	GPIO_PIN30, GPIO_PIN31,
};

/* Each pin can interrupt on one of four different events */
enum gpio_interrupt_mode {
	GPIO_IMODE_LOW_LEVEL		= 0x0,
	GPIO_IMODE_HIGH_LEVEL		= 0x1,
	GPIO_IMODE_FALLING_EDGE		= 0x2,
	GPIO_IMODE_RISING_EDGE		= 0x3,
};

/* Each pin can drive with configurable current */
enum gpio_current {
	GPIO_CURRENT_2MA		= 0x0,
	GPIO_CURRENT_4MA		= 0x1,
	GPIO_CURRENT_6MA		= 0x2,
	GPIO_CURRENT_8MA		= 0x3,
};

/* virtual name for each gpio port/pin function */
enum gpio_resource {
	HEADPHONE_JACK			= 0,
	LED_ENA				= 1,
	LCD_RESET			= 2,
	AUDIO_POWER			= 3,
	DPAD_UP				= 4,
	DPAD_DOWN			= 5,
	DPAD_RIGHT			= 6,
	DPAD_LEFT			= 7,
	BUTTON_A			= 8,
	BUTTON_B			= 9,
	SHOULDER_LEFT			= 10,
	SHOULDER_RIGHT			= 11,
	BUTTON_HOME			= 12,
	BUTTON_HINT			= 13,
	BUTTON_PAUSE			= 14,
	BUTTON_BRIGHTNESS		= 15,
	BUTTON_VOLUMEUP			= 16,
	BUTTON_VOLUMEDOWN		= 17,
	CARTRIDGE_DETECT		= 18,
	TOUCHSCREEN_X1			= 19,
	TOUCHSCREEN_Y1			= 20,
	TOUCHSCREEN_X2			= 21,
	TOUCHSCREEN_Y2			= 22,
	BUTTON_RED			= 23,
	GPIO_NUMBER_VALUES		= 24,
};

/*
 * Scratchpad Register usage
 */

#define SCRATCH_POWER_POS          0
#define SCRATCH_POWER_SIZE         2

#define SCRATCH_SHUTDOWN_POS       2
#define SCRATCH_SHUTDOWN_SIZE      1

#define SCRATCH_REQUEST_POS 	   3
#define SCRATCH_REQUEST_SIZE  	   3

#define SCRATCH_BOOT_IMAGE_POS     6
#define SCRATCH_BOOT_IMAGE_SIZE    2

#define SCRATCH_BOARD_ID_POS       8
#define SCRATCH_BOARD_ID_SIZE      5

#define SCRATCH_CART_ID_POS       13
#define SCRATCH_CART_ID_SIZE       4

#define SCRATCH_BOOT_SOURCE_POS   17
#define SCRATCH_BOOT_SOURCE_SIZE   3

#define SCRATCH_PANIC_POS	  20
#define SCRATCH_PANIC_SIZE         2

#define SCRATCH_USER_0_POS        22
#define SCRATCH_USER_0_SIZE       (32-SCRATCH_USER_0_POS)

/*
 * SCRATCHPAD Enums
 * Note that scratchpad register is cleared to zero on first power up
 */

/* Track power state */
enum scratch_power {                 // set by bootstrap, read by others
        SCRATCH_POWER_FIRSTBOOT = 0, // only seen in bootstrap
        SCRATCH_POWER_COLDBOOT  = 1, // batteries replaced
        SCRATCH_POWER_WARMBOOT  = 2, // second and subsequent boots
};

/* Shutdown semaphore.  Set by Linux shutdown to signal clean system shutdown */
enum scratch_shutdown {
        SCRATCH_SHUTDOWN_CLEAN = 0,  // set by Linux at clean shutdown
        SCRATCH_SHUTDOWN_DIRTY = 1,  // cleared by bootstrap at boot
};

/* Choose boot partition.  Used by REQUEST_BOOT and ACTUAL_BOOT bits to
 * indicate the preferred boot partition and the partition booted
 */
enum scratch_boot_image {
        SCRATCH_BOOT_IMAGE_RECOVERY = 0,   // boot recovery image
        SCRATCH_BOOT_IMAGE_PLAY     = 1,   // normal boot
        SCRATCH_BOOT_IMAGE_2        = 2,   // unused
        SCRATCH_BOOT_IMAGE_3        = 3,   // unused
};

/* Choose boot partition.  Used by REQUEST_BOOT and ACTUAL_BOOT bits to
 * indicate the preferred boot partition and the partition booted
 */
enum scratch_boot_source {
        SCRATCH_BOOT_SOURCE_UNKNOWN = 0,   // 
        SCRATCH_BOOT_SOURCE_NOR     = 1,   // 
        SCRATCH_BOOT_SOURCE_NAND    = 2,   // 
        SCRATCH_BOOT_SOURCE_UART    = 3,   // 
        SCRATCH_BOOT_SOURCE_USB     = 4,   // 
};

/* save boot source. */
enum scratch_request {
	SCRATCH_REQUEST_PLAY    = 0,  // Launch Play if possible
	SCRATCH_REQUEST_RETURN  = 1,  // Return to Play if possible
	SCRATCH_REQUEST_UPDATE  = 2,  // Enter recovery in update mode
	SCRATCH_REQUEST_BATTERY = 3,  // Enter play: battery failed
	SCRATCH_REQUEST_UNCLEAN = 4,  // Enter play: dirty shutdown
	SCRATCH_REQUEST_FAILED  = 5,  // Enter recovery in update mode: boot failed
	SCRATCH_REQUEST_SHORT   = 6,  // Enter play in short-circuit mode
	SCRATCH_REQUEST_TRAPDOOR= 7,  // Enter recovery in trapdoor mode
};

// Priority: _TRAPDOOR, _SHORT, _FAILED, _UNCLEAN, _BATTERY, _UPDATE, _RETURN, _PLAY


void gpio_set_val(enum gpio_port port, enum gpio_pin pin, u8 en);
int gpio_get_val(enum gpio_port port, enum gpio_pin pin);
void gpio_configure_pin(enum gpio_port port, enum gpio_pin pin, 
		enum gpio_function f, u8 out_en, u8 pu_en, u8 val);
unsigned long gpio_get_cur(enum gpio_port port, enum gpio_pin pin);
void gpio_set_cur(enum gpio_port port, enum gpio_pin pin, enum gpio_current cur);
u32 gpio_get_scratchpad(void);
void gpio_set_scratchpad(u32 val);
void gpio_set_out_en(enum gpio_port port, enum gpio_pin pin, unsigned char en);
#endif /* __GPIO_H__ */
