/* dpc.h  -- prototype of the display controller driver initialization function
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DPC_H__
#define __DPC_H__

#include <display.h>

void dpc_enable(void);
void dpc_init(const struct display_module *disp);

#endif /* __DPC_H__ */
