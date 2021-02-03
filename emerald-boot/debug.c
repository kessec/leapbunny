/* debug.c -- Simple UART driver for debugging.
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * This file contains code for five groups of functions:
 *	db_init()	    this function configures the LF1000's uart as
 *			    indicated by the kernel configuration.  When the
 *			    boot code was built with DEBUG defined, it also
 *			    initializes the idb_* framework (see below).
 *	serio_* functions:  these write to the serial port whether or not the
 *			    boot code was built with DEBUG defined.
 *	db_* functions:     these write to the serial port if the boot code 
 *			    was built with DEBUG defined.  
 *	idb_* functions:    these are similar to the db_* functions, but they
 *			    are designed to be called from interrupt service
 *			    routines.  Instead of writing directly to the 
 *			    serial port, they write to a circular buffer.
 *			    The background (non-interrupt) loop ought to call
 *			    idb_output() to move characters from the circular
 *			    buffer to the serial port.
 *	db_stopwatch_* functions: these start and stop a timer.
 *
 * NOTE:    All of the boot code's serial output is performed by serio_putchar().
 *	    This function does not write a byte to the uart until the uart's
 * transmitter is idle.  When it is called to output several bytes in close
 * succession, this can result in significant delays.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <base.h>
#include <gpio.h>
#include <gpio_hal.h>
#include <debug.h>

#define SYS_UART_BASE	UART0_BASE
#define UART_PLL	1
#define UARTDIV		40

#define UART8(r)	REG8(SYS_UART_BASE+r)
#define UART16(r)	REG16(SYS_UART_BASE+r)
#define UART32(r)	REG32(SYS_UART_BASE+r)

#if defined DEBUG && defined DEBUG_STOPWATCH
#include <timer.h>
const char *db_stopwatch;
#else
#define timer_init()
#endif

#ifdef DEBUG
static void idb_init();
#else
#define idb_init()
#endif

#define LCON		0x00
#define UCON		0x02
#define TRSTATUS	0x08
#define FSTATUS		0x0C
#define THB		0x10
#define RHB		0x12
#define BRD		0x14
#define UARTCLKGEN	0x44

/* LCON Register */
#define SYNC_PENDCLR	7
#define WORD_LEN	0
/* UCON Register */
#define TX_INT		9
#define RX_INT		8
#define RX_TIMEOUT	7
#define RX_ERRSTATUS	6
#define LOOPBACK_MODE	5
#define SEND_ BREAK	4
#define TRANS_MODE	2
#define RECEIVE_MODE	0

/* UARTCLKGEN */
#define UARTCLKDIV	4
#define UARTCLKSRCSEL	1

/* TRSTATUS */
#define TRANSMIT_BUFFER_EMPTY		1
#define RECEIVE_BUFFER_DATA_READY	0


/*
 * Initialize UART, return prior UART bit rate setting
 */
u16 db_init(void)
{
	u16 ret_bitrate;

	/* GPIO setup (for UART0) */
	gpio_configure_pin(GPIO_PORT_A, GPIO_PIN8, GPIO_ALT1, 1, 0, 0); /*TX*/

	/* clear IRQ pending, set 8 bit word length */
	UART16(LCON) = (1<<SYNC_PENDCLR)|(3<<WORD_LEN);

	/* enable polling/IRQ transmit, leave receive disabled */
	UART16(UCON) = (1<<TRANS_MODE)|(1<<RECEIVE_MODE);

	/* set the baud rate */
	ret_bitrate = UART16(BRD);	// return prior bit rate setting
	UART16(BRD) = 1; /* FIXME (for now this sets 115200 baud) */
	UART16(UARTCLKGEN) = ((UARTDIV-1)<<UARTCLKDIV)|(UART_PLL<<UARTCLKSRCSEL);

	timer_init();
	idb_init();

	return ret_bitrate;
}

#ifndef NO_SERIAL_OUT 		// 25mar11
int serio_getchar(char *c)
{
	if (!(UART16(FSTATUS) & 0xF))
	       return 0;

	*c = UART16(RHB);
	return 1;
}

/* serio_putchar() is a renamed copy of db_putchar().  
 * It was created to allow output of a boot-code version string
 * even when the boot code is built without DEBUG defined.
 */
void serio_putchar(char c)
{
	if(c == '\n')
		serio_putchar('\r');
	while(IS_CLR(UART16(TRSTATUS), TRANSMIT_BUFFER_EMPTY));
	UART8(THB) = c;
}

/* serio_puts() is a renamed copy of db_puts().  
 * It was created to allow output of a boot-code version string
 * even when the boot code is built without DEBUG defined.
 */
void serio_puts(const char *s)
{
	do {
		serio_putchar(*s++);
	} while(*s != 0);
}

void serio_hex(unsigned int val)
{
	serio_putchar ("0123456789ABCDEF"[val & 0xf]);
}

void serio_int(unsigned int val)
{
	serio_hex(val>>(4*7));
	serio_hex(val>>(4*6));
	serio_hex(val>>(4*5));
	serio_hex(val>>(4*4));
	serio_hex(val>>(4*3));
	serio_hex(val>>(4*2));
	serio_hex(val>>(4*1));
	serio_hex(val>>(4*0));
}

void serio_byte(unsigned char val)
{
	serio_hex(val>>4);
	serio_hex(val);
}
#endif	// NO_SERIAL_OUT  25mar11

#ifdef DEBUG

void db_byte(char val)
{
	db_hex(val>>4);
	db_hex(val & 0xF);
}

void db_16(u16 val)
{
	db_puts("0x");
	db_hex(val>>(4*3));
	db_hex(val>>(4*2));
	db_hex(val>>(4*1));
	db_hex(val>>(4*0));
}

void db_int(unsigned int val)
{
	db_puts("0x");
	db_hex(val>>(4*7));
	db_hex(val>>(4*6));
	db_hex(val>>(4*5));
	db_hex(val>>(4*4));
	db_hex(val>>(4*3));
	db_hex(val>>(4*2));
	db_hex(val>>(4*1));
	db_hex(val>>(4*0));
}

/*
 * Timer stopwatch debugging
 */

#ifdef DEBUG_STOPWATCH

void db_stopwatch_start(const char *name)
{
	db_stopwatch = name;
	db_puts("START :: ");
	db_puts(db_stopwatch);
	db_putchar('\n');
	timer_start();
}

void db_stopwatch_stop(void)
{
	db_puts("STOP  :: ");
	db_puts(db_stopwatch);
	db_puts(" :: ");
	db_int(timer_stop());
	db_putchar('\n');
}
#endif /* DEBUG_STOPWATCH */

	//--------------------------------------------------------------------------
	// The idb_XXX() routines (except for idb_output()) are intended to be
	// called from interrupt service routines.  Instead of writing directly
	// to the uart, they store characters in debug_buffer[], which is used
	// as a circular buffer.
	// The background loop calls idb_output() to fetch bytes from debug_buffer[]
	// and write them to the uart.
	//
	// Each of dbCtrl's members is initialized to 0.  
	// Whenever a character is stored in debug_buffer[ dbCtrl.in ], 
	// dbCtrl.in is incremented (and wrapped if necessary) and dbCtrl.count
	// is incremented.
	// Whenever a character is fetched from debug_buffer[ dbCtrl.out ],
	// dbCtrl.out is incremented (and wrapped if necessary), and dbCtrl.count
	// is decremented.
	// When cbCtrl.count is zero, there's nothing to put out.
	// If dbCtrl.count equals the size of debug_buffer[], there is no room
	// to add another character.  In that case characters are simply discarded.
	//--------------------------------------------------------------------------
static char debug_buffer[4096]; // code assumes the length is a power of 2.

static volatile struct tagDBCtrl {
	int		in;		// index where the next added character ought to be put
	int		out;	// index where the next fetched character ought to come from
	int		count;	// number of bytes in debug_buffer that need to be output
} dbCtrl;

static void idb_init() {
	dbCtrl.in	 = 0;
	dbCtrl.out	 = 0;
	dbCtrl.count = 0;
}

void idb_putchar(char c)
{
	if(c == '\n')
		idb_putchar('\r');

	if (dbCtrl.count >= sizeof(debug_buffer))
		return;

	debug_buffer[ dbCtrl.in ] = c;
	dbCtrl.in	 = (1 + dbCtrl.in) & (sizeof(debug_buffer) - 1);
	dbCtrl.count = 1 + dbCtrl.count;
}

void idb_puts(const char *s)
{
	do {
		idb_putchar(*s++);
	} while(*s != 0);
}

void idb_hex(unsigned int val)
{
	val= val & 0xf;
	if (val <10)
		idb_putchar('0'+val);
	else
		idb_putchar('A'+(val-10));
}

void idb_byte(char val)
{
	idb_hex(val>>4);
	idb_hex(val & 0xF);
}

void idb_16(u16 val)
{
	idb_puts("0x");
	idb_hex(val>>(4*3));
	idb_hex(val>>(4*2));
	idb_hex(val>>(4*1));
	idb_hex(val>>(4*0));
}

void idb_int(unsigned int val)
{
	idb_puts("0x");
	idb_hex(val>>(4*7));
	idb_hex(val>>(4*6));
	idb_hex(val>>(4*5));
	idb_hex(val>>(4*4));
	idb_hex(val>>(4*3));
	idb_hex(val>>(4*2));
	idb_hex(val>>(4*1));
	idb_hex(val>>(4*0));
}

void DisableInterrupts();
void EnableInterrupts();
void idb_output() {
	while (dbCtrl.count > 0) {
		db_putchar(debug_buffer[dbCtrl.out]);
		DisableInterrupts();	// don't let idb_putchar() change it at same time
		dbCtrl.count = dbCtrl.count - 1;
		EnableInterrupts();
		dbCtrl.out	 = (1 + dbCtrl.out) & (sizeof(debug_buffer) - 1);
	}
}

#endif	/* DEBUG */

