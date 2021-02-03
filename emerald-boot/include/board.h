/* board.h  -- board configuration
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BOARD_H__
#define __BOARD_H__

/*
 * This is a list of board types that can be detected at runtime to deal
 * with hardware quirks. See gpio_get_board_config() in LinuxDist for
 * more information.
 */

/* Machine Types that we support */

#define	MACH_TYPE_DIDJ			2028

/* Machine Type to boot */

#define MACH_TYPE			MACH_TYPE_DIDJ

/* LF1000 Development boards and original Form Factor (alpha) board. */
#define LF1000_BOARD_DEV                0x00

/* EP1 Form Factor Board */
#define LF1000_BOARD_DIDJ               0x03

/* DIDJ 09 Board */
#define LF1000_BOARD_DIDJ_09            0x04

/* Acorn / Leapster3 Board */
#define LF1000_BOARD_ACORN              0x05

/* Emerald / Leapster3 Board */
#define LF1000_BOARD_EMERALD_POP	0x01
#define LF1000_BOARD_EMERALD_NOTV_NOCAP 0x02
#define LF1000_BOARD_EMERALD_TV_NOCAP   0x06
#define LF1000_BOARD_EMERALD_NOTV_CAP   0x07
#define LF1000_BOARD_EMERALD_SAMSUNG	0x0A

/* K2 Board */
#define LF1000_BOARD_K2			0x10

/* Madrid Board */
#define LF1000_BOARD_MADRID		0x0B
#define LF1000_BOARD_MADRID_POP		0x0C
#define LF1000_BOARD_MADRID_LFP100	0x0D

/*
 * minimum voltage for booting (with backlight on), in mV
 */
#define BOOT_MIN_MV	4000	/* below this the system may continuously reboot */
#define BOOT_LOW_BATT	4400	/* warn the batteries are low			 */

/*
 * Initial CPU speed
 */

#define PLL0_M		801
#define PLL0_P		55
#define PLL0_S		0

/* 
 * File System Locations
 */

#define P0_SIZE 0x00100000 // Boot
#define P1_SIZE 0x00100000 // I18n
#define P2_SIZE 0x00800000 // Kernel

#define	EBS_64	0x00010000
#define	EBS_128	0x00020000
#define	EBS_256	0x00040000
#define	EBS_512	0x00080000

#define BOOT_FLAGS_ADDR(e)	(P0_SIZE)
#define BOOT_FLAGS_SIZE(e)	(P1_SIZE)
#define BOOT0_ADDR(e)	(P0_SIZE + P1_SIZE)
#define BOOT_SIZE	(P2_SIZE)

/* memory reserved for frame buffers (graphics) */
#define BOARD_MLC_RESERVE_MEMORY	18

/* 
 * Kernel Command Line settings 
 *
 */

/* Base command line */
/* loglevel=7 is lots of boot messages */
#define CMDLINE_BASE "init=/sbin/init console=ttyS0,115200 loglevel=2 "

/* command line rootfs components */
#define CMDLINE_RFS "root=ubi0_0 rw rootfstype=ubifs ubi.mtd=RFS "
#define CMDLINE_RFS_SD0 "root=/dev/mmcblk0p3 rootwait "
#define CMDLINE_RFS_SD1 "root=/dev/mmcblk0p3 rootwait "

/*
 * Display and boot splash settings
 */
// #define FB_VIRTUAL_XRES		320
// #define FB_VIRTUAL_YRES		240
#define FRAME_BUFFER_SIZE	(FB_VIRTUAL_XRES*FB_VIRTUAL_YRES*3)
#define BOOTSPLASH_NAME		"bootsplash.rgb"

/*
 * Internationalization Support
 */
#define CMDLINE_LANGUAGE "language="
#define CMDLINE_COUNTRY "country="

/* Battery voltage sensing ADC channel */
#define BOARD_ADC_VBATSENSE	2

#endif /* __BOARD_H__ */
