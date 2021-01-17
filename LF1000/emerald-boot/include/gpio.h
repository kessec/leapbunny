/* gpio.h  -- configure and access LF1000 GPIOs
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_GPIO_H
#define LIGHTNING_BOOT_GPIO_H

void gpio_set_val(enum gpio_port port, enum gpio_pin pin, u8 en);
int gpio_get_val(enum gpio_port port, enum gpio_pin pin);
void gpio_configure_pin(enum gpio_port port, enum gpio_pin pin, 
		enum gpio_function f, u8 out_en, u8 pu_en, u8 val);
u32 gpio_get_scratchpad(void);
void gpio_set_scratchpad(u32 val);
#endif /* LIGHTNING_BOOT_GPIO_H */
