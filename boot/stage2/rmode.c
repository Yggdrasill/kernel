/*
 * MBR bootloader, currently unnamed
 * Copyright (C) 2017  Yggdrasill <kaymeerah@lambda.is>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 */

#include "stdint.h"
#include "string.h"

#include "rmode.h"

#include "idt.h"
#include "interrupt.h"
#include "mmap.h"

struct mmap_array *__bios_mmap(void);
void               __bios_print(uint16_t str, uint16_t len);

/*
 * rmode_trampoline cannot directly return union because it invokes sret stack
 * behaviour, which then causes rmode_trampoline to pop the wrong value off the
 * stack. This will proceed to cause triple-faulting as a return is made to a
 * bogus address.
 */

extern uint32_t rmode_trampoline(void (*)(void), ...);

struct mmap_array *bios_mmap(void)
{
	union rmode_ret_t rv;
	rv.u32 = rmode_trampoline((void (*)(void))__bios_mmap);
	return rv.ptr;
}

void bios_print(char *str, size_t len)
{
	rmode_trampoline((void (*)(void))__bios_print, str, len);
	return;
}
