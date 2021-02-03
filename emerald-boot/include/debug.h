/* debug.h  -- UART debug output functions
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This file contains prototypes for five groups of functions:
 *      db_init()           this function configures the LF1000's uart as
 *                          indicated by the kernel configuration.  When the
 *                          boot code was built with DEBUG defined, it also
 *                          initializes the idb_* framework (see below).
 *      serio_* functions:  these write to the serial port whether or not the
 *                          boot code was built with DEBUG defined.
 *      db_* functions:     these write to the serial port if the boot code 
 *                          was built with DEBUG defined.  
 *      idb_* functions:    these are similar to the db_* functions, but they
 *                          are designed to be called from interrupt service
 *                          routines.  Instead of writing directly to the 
 *                          serial port, they write to a circular buffer.
 *                          The background (non-interrupt) loop ought to call
 *                          idb_output() to move characters from the circular
 *                          buffer to the serial port.
 *      db_stopwatch_* functions: these start and stop a timer.
 *
 * NOTE:    All of the boot code's serial output is performed by serio_putchar().
 *          This function does not write a byte to the uart until the uart's
 * transmitter is idle.  When it is called to output several bytes in close
 * succession, this can result in significant delays.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_UART_H
#define LIGHTNING_BOOT_UART_H

u16 db_init(void);
//#define NO_SERIAL_OUT  
#ifdef NO_SERIAL_OUT	// 25mar11
#define serio_puts(...) 
#define serio_putchar(...) 
#define serio_getchar(...) 
#define serio_hex(...) 
#define serio_int(...) 
#define serio_byte(...) 
#else	// 25mar11

void serio_puts(const char *s);
void serio_putchar(char c);
int serio_getchar(char *c);
void serio_hex(unsigned int val);
void serio_int(unsigned int val);
void serio_byte(unsigned char val);
#endif	// 25mar11

#ifdef DEBUG
#define db_putchar(c) serio_putchar(c)
#define db_puts(s)    serio_puts(s)
#define db_hex(val)   serio_hex(val)
void db_byte(char val);
void db_16(u16 val);
void db_int(unsigned int val);

void idb_putchar(char c);
void idb_puts(const char *s);
void idb_hex(unsigned int val);
void idb_byte(char val);
void idb_16(u16 val);
void idb_int(unsigned int val);
void idb_output();

#else
#define db_putchar(...)
#define db_puts(...)
#define db_hex(...)
#define db_byte(...)
#define db_16(...)
#define db_int(...)

#define idb_putchar(c)   
#define idb_puts(s)   
#define idb_hex(c)  
#define idb_byte(c)  
#define idb_16(v)  
#define idb_int(v)  
#define idb_output()  

#endif /* DEBUG */

/* timer stopwatch */

#if defined DEBUG && defined DEBUG_STOPWATCH
void db_stopwatch_start(const char *name);
void db_stopwatch_stop(void);
#else
#define db_stopwatch_start(...)
#define db_stopwatch_stop(...)
#endif /* DEBUG && DEBUG_STOPWATCH */

#endif /* LIGHTNING_BOOT_UART_H */
