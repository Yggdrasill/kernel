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

#include "gdt.h"
#include <libk/gdt.h>
#include <libk/internal/gdt.h>

extern struct gdt_ptr   __GDTR_DATA;
extern struct gdt_entry __GDT_ENTRIES[];

static struct gdt_info gdt_info;

struct gdt_info *gdt_info_init(void)
{
    gdt_info = (struct gdt_info){
        .gdtr           = &__GDTR_DATA,
        .entries        = __GDT_ENTRIES,
        .max_nr_entries = GDT_MAX_ENTRIES,
        .nr_entries     = 0,
    };
    return &gdt_info;
}
