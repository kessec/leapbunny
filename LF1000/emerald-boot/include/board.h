/* board.h  -- board configuration
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_CONFIG_H
#define LIGHTNING_BOOT_CONFIG_H

#include <mach/platform.h>
#include <mach/mlc.h>

/*
 * This is a list of board types that can be detected at runtime to deal
 * with hardware quirks. See gpio_get_board_config() in LinuxDist for
 * more information.
 */

/* LF1000 Development boards and original Form Factor (alpha) board. */
#define LF1000_BOARD_DEV                0x00

/* EP1 Form Factor Board */
#define LF1000_BOARD_DIDJ               0x03

/* DIDJ 09 Board */
#define LF1000_BOARD_DIDJ_09            0x04

/* Acorn / Leapster3 Board */
#define LF1000_BOARD_ACORN              0x05

/* Emerald / Leapster3 Board */
#define LF1000_BOARD_EMERALD_NOTV_NOCAP 0x02
#define LF1000_BOARD_EMERALD_TV_NOCAP   0x06
#define LF1000_BOARD_EMERALD_NOTV_CAP   0x07

#define LF1000_BOARD_K2			0x10

/*
 * minimum voltage for booting (with backlight on), in mV
 */
#define BOOT_MIN_MV	4400
#define LOW_BATT_PORT	GPIO_PORT_C
#define LOW_BATT_PIN	GPIO_PIN14


/* 
 * File System Locations (from autoconf.h)
 * - BOOT_FLAGS contains the control file and other bootup data
 * - BOOT0 and BOOT1 contain the kernel image
 */

#define P0_SIZE CONFIG_NAND_LF1000_P0_SIZE	// Boot
#define P1_SIZE CONFIG_NAND_LF1000_P1_SIZE	// I18n
#define P2_SIZE CONFIG_NAND_LF1000_P2_SIZE	// Kernel

#define	EBS_64	0x00010000
#define	EBS_128	0x00020000
#define	EBS_256	0x00040000
#define	EBS_512	0x00080000

#define BOOT_FLAGS_ADDR(e)	(P0_SIZE)
#define BOOT_FLAGS_SIZE(e)	(P1_SIZE)
#define BOOT0_ADDR(e)	(P0_SIZE + P1_SIZE)
#define BOOT_SIZE	(P2_SIZE)

/* 
 * Kernel Command Line settings 
 *
 */

/* Base command line */
#define CMDLINE_BASE "init=/sbin/init console=ttyS0,115200 loglevel=2 "

/* command line rootfs components */
#define CMDLINE_RFS "root=ubi0_0 rw rootfstype=ubifs ubi.mtd=RFS "

/*
 * Display and boot splash settings
 */
#define X_RESOLUTION		320
#define Y_RESOLUTION		240
#define FRAME_BUFFER_SIZE	(X_RESOLUTION*Y_RESOLUTION*3)
#define BOOTSPLASH_NAME		"bootsplash.rgb"

/*
 * u-boot settings
 */
#ifdef UBOOT_SUPPORT
/* PORTC button masks */
#if defined CONFIG_MACH_ME_LF1000 || defined CONFIG_MACH_LF_LF1000
#define BUTTON_MSK      0x00003F9F
#else /* LF_ME2530 and ME_ME2530 boards */
#define BUTTON_MSK      0x1FE00000
#endif
#define UBOOT_ADDR	0x01000000
#endif /* UBOOT_SUPPORT */

/*
 * Internationalization Support
 */
#define CMDLINE_LANGUAGE "language="
#define CMDLINE_COUNTRY "country="

#endif /* LIGHTNING_BOOT_CONFIG_H */
