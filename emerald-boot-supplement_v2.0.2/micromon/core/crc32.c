/*
 * Copyright (C) 2011 Jeff Kent <jakent@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "crc32.h"

u32 crc32_table[256];


u32 reflect(u32 n, u8 bits)
{
	u32 in_mask = 1 << (bits - 1);
	u32 out_mask = 1;
	u32 n_reflected = 0;
	
	while (in_mask) {
		if (n & in_mask)
			n_reflected |= out_mask;
		out_mask <<= 1;
		in_mask >>= 1;
	}

	return n_reflected;
}


void init_crc32_table(void)
{
	u32 crc;
	u16 i;
	u8 j;

	for (i = 0; i < 256; i++) {
		crc = reflect(i, 8) << 24;
		for (j = 0; j < 8; j++) {
			if (crc & 0x80000000) {
				crc = (crc << 1) ^ CRC32_POLYNOMIAL;
			} else {
				crc = (crc << 1);
			}
		}
		crc32_table[i] = reflect(crc, 32);
	}
}


u32 crc32(u32 crc, const u8 data)
{
	u8 i;

	crc ^= 0xFFFFFFFF;
    i = (crc ^ data) & 0xFF;
	crc = (crc >> 8) ^ crc32_table[i];
	return crc ^ 0xFFFFFFFF;
}



