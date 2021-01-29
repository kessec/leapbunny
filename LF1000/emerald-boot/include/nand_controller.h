/* NAND Flash Controller
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 */

#ifndef __NAND_CONTROLLER_H__
#define __NAND_CONTROLLER_H__

/* registers as offsets from MCU_S_BASE */
#define NFCONTROL		0x74
#define NFECCL			0x78
#define NFECCH			0x7C
#define NFORGECCL		0x80
#define NFORGECCH		0x84
#define NFCNT			0x88
#define NFECCSTATUS		0x8C
#define NFSYNDRONE31		0x90
#define NFSYNDRONE75		0x94

/* registers as offsets from NAND_BASE */
#define NFDATA			0x00
#define NFCMD			0x10
#define NFADDR			0x18

/* NAND Flash Control Register (NFCONTROL) */
#define IRQPEND			15
#define ECCRST			11
#define RnB			9
#define IRQENB			8
#define NFBOOTENB		5
#define NFTYPE			3
#define NFBANK			0

/* NFTYPE bits in NFCONTROL */
#define NFTYPE_LBLOCK		4	/* large block Flash */
#define NFTYPE_EADDR		3	/* extra address Flash */

/* NAND Flash Data Count Register (NFCNT) */
#define NFWRCNT			16
#define NFRDCNT			0

/* NAND FLASH ECC STATUS REGISTER (NFECCSTATUS) */
#define NFCHECKERROR		2
#define NFECCDECDONE		1	/* (reading done) */
#define NFECCENCDONE		0	/* (writing done) */

/* NAND FLASH ECC SYNDROME VALUE31 REGISTER (NFSYNDRONE31) */
#define SYNDROM3		13
#define SYNDROM1		0

/* NAND FLASH ECC SYNDROME VALUE75 REGISTER (NFSYNDRONE75) */
#define SYNDROM7		13
#define SYNDROM5		0

#endif /* __NAND_CONTROLLER_H__ */
