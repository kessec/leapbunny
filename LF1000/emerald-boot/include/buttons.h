/* buttons.h -- Button mapping API.
 *
 * Copyright 2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BUTTONS_H__
#define __BUTTONS_H__

struct buttons_state {
	unsigned a		: 1;
	unsigned b		: 1;
	unsigned ls		: 1;
	unsigned rs		: 1;
	unsigned p		: 1;
	unsigned hint	: 1;
	unsigned up		: 1;
	unsigned down	: 1;
};

void buttons_get_state(int board_id, struct buttons_state *b);

#endif /* __BUTTONS_H__ */
