/* vim: set ts=4 sw=4 noexpandtab
 *
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

#include <asm/arch/platform.h>
#include <asm/arch/common.h>
#include "startup.h"
#include "uart.h"
#include "crc32.h"
#include "loader.h"
#include "core.h"

inline void core_loop(void);
inline void core_mem_write(void);
inline void core_mem_read(void);
inline void core_write_u8();
inline void core_write_u16();
inline void core_write_u32();
inline void core_read_u8();
inline void core_read_u16();
inline void core_read_u32();

enum core_cmd_commands {
	core_cmd_nop = 0,
	core_cmd_set_baudrate,
	core_cmd_write_u8,
	core_cmd_write_u16,
	core_cmd_write_u32,
	core_cmd_read_u8,
	core_cmd_read_u16,
	core_cmd_read_u32,
	core_cmd_mem_write,
	core_cmd_mem_read,
	core_cmd_run,
	core_cmd_run_kernel,
	END_OF_COMMANDS
};

void main(void)
{
	u32 crc = 0;

	init_crc32_table();

	for (u32 addr = CORE_START; addr < (CORE_START+CORE_SIZE); addr++) {
		u8 *p = (u8 *)addr;
		crc = crc32(crc, *p);
	}

	put_u32(crc);

	core_loop();
}

inline void core_loop(void)
{
	u8 module, command;

	while (1) {
		module = getc();
		if (module == 0) {
			command = getc();
			switch (command) {
			case core_cmd_nop:
				break;

			case core_cmd_set_baudrate:
				loader_set_baudrate(get_u32());
				break;

			case core_cmd_write_u8:
				core_write_u8();
				break;

			case core_cmd_write_u16:
				core_write_u16();
				break;

			case core_cmd_write_u32:
				core_write_u32();
				break;

			case core_cmd_read_u8:
				core_read_u8();
				break;

			case core_cmd_read_u16:
				core_read_u16();
				break;

			case core_cmd_read_u32:
				core_read_u32();
				break;

			case core_cmd_mem_write:
				core_mem_write();
				break;

			case core_cmd_mem_read:
				core_mem_read();
				break;

			case core_cmd_run:
				core_run(get_u32());
				break;

			case core_cmd_run_kernel:
				core_run_kernel(get_u32(), get_u32());
				break;
			}
		}
	}
}

inline void core_read_u8(void)
{
	u8 *addr = (u8 *)get_u32();
	putc(*addr);
}

inline void core_read_u16(void)
{
	u16 *addr = (u16 *)get_u32();
	put_u16(*addr);
}

inline void core_read_u32(void)
{
	u32 *addr = (u32 *)get_u32();
	put_u32(*addr);
}

inline void core_write_u8(void)
{
	u8 *addr = (u8 *)get_u32();
	*addr = getc();
}

inline void core_write_u16(void)
{
	u16 *addr = (u16 *)get_u32();
	*addr = get_u16();
}

inline void core_write_u32(void)
{
	u32 *addr = (u32 *)get_u32();
	*addr = get_u32();
}

inline void core_mem_write(void)
{
	u8 *addr, *p;
	u32 size;
	u32 crc = 0;

	addr = (u8 *)get_u32();
	size = get_u32();

	p = addr;
	while ((u32)addr + size > (u32)p) {
		*p = getc();
		crc = crc32(crc, *p);
		p++;
	}
	put_u32(crc);
}

inline void core_mem_read(void)
{
	u8 *addr, *p;
	u32 size;
	u32 crc = 0;

	addr = (u8 *)get_u32();
	size = get_u32();

	p = addr;
	while ((u32)addr + size > (u32)p) {
		putc(*p);
		crc = crc32(crc, *p);
		p++;
	}
	put_u32(crc);
}

void core_run(u32 exec_at)
{
	((void(*)())exec_at)();
}

void core_run_kernel(u32 exec_at, u32 machine_type)
{
	void (*kernel)(int zero, int arch, u32 params);
	u32 param_at = (u32)-1;

	kernel = (void (*)(int, int, u32))exec_at;
	kernel(0, machine_type, param_at);
}
