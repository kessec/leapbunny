/* i2c.h - Basic polling I2C driver
 *
 * Copyright 2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __I2C_H__
#define __I2C_H__

/* Initialize I2C controller channel 'ch' pins.
 *
 * Returns 0 on success and 1 if 'ch' is not valid.*/
int i2c_init(u8 ch);

/* Read from 'len' bytes into 'buf' from an I2C slave at address 'addr' on I2C
 * channel 'ch'. 
 *
 * Returns 0 on success, 1 if the 'ch' argument is not valid, 2 if we did not
 * receive an ACK, or 3 if we timed out while waiting for an IRQ. */
int i2c_read(u8 ch, u8 addr, u8 *buf, unsigned int len);

/* Write the contents of 'buf', length 'len', to an I2C slave at address 'addr'
 * on I2C channel 'ch'.
 *
 * Returns 0 on success, 1 if the 'ch' argument is not valid, 2 if we did not
 * receive an ACK, or 3 if we timed out while waiting for an IRQ. */
int i2c_write(u8 ch, u8 addr, u8 *buf, unsigned int len);

#endif /* __I2C_H__ */
