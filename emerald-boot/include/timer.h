/* timer.h -- timer support
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_TIMER_H
#define LIGHTNING_BOOT_TIMER_H

void timer_init(void);
void timer_start(void);
u32  timer_stop(void);

#endif /* LIGHTNING_BOOT_TIMER_H */
