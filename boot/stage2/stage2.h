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

#ifndef STAGE2_H
#define STAGE2_H

#include <stddef.h>
#include <stdint.h>

struct mem_extent {
    size_t start;
    size_t end;
};

/*
 * Preallocated memory regions for various sections, once again see
 * boot/common/linker.lds.S.
 */

extern char __BIOS_START;
extern char __BIOS_END;
extern char __BOOTLOADER_START;
extern char __BOOTLOADER_END;

extern char __PREALLOC_START;
extern char __PREALLOC_END;
extern char __STACK_START;
extern char __STACK_END;

extern char __FB_ADDR;
extern char __FB_END;
extern char __UPPER_START;
extern char __UPPER_END;

#endif
