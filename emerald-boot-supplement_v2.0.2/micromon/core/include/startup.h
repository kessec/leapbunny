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

#ifndef __STARTUP_H__
#define __STARTUP_H__
 
/**************************************************************************
 *
 *         Memory layout
 *   0x01F00000  .-----------.
 *               | 16K Core  |
 *               | Code      |
 *   0x01F04000  |-----------|
 *               | 768K Mod  |
 *               | Code      |
 *   0x01FC4000  |-----------|
 *               | 128K Mod  |
 *               | Data      |
 *   0x01FE4000  |-----------|
 *               | 48K Core  |
 *               | BSS       |
 *   0x01FF0000  |-----------|
 *               | 64K Share |
 *               | Stack     |
 *   0x02000000  '-----------'
 *
 *************************************************************************/

#define MICROMON_START (1024 * 1024 * 31)

#define MICROMON_SIZE  (1024 * 1024 * 1)
#define CORE_SIZE      (1024 * 16)
#define BSS_SIZE       (1024 * 48)
#define STACK_SIZE     (1024 * 64)

#define MICROMON_END   (MICROMON_START + MICROMON_SIZE)

#define CORE_START		MICROMON_START
#define CORE_END		(CORE_START + CORE_SIZE)

#define MODULES_START  CORE_END

#define STACK_START    MICROMON_END
#define STACK_END      (MICROMON_END - STACK_SIZE)

#define BSS_END        STACK_END
#define BSS_START      (BSS_END - BSS_SIZE)

#define MODULES_END    BSS_START

#endif /*__STARTUP_H__*/
