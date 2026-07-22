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

#include "idt.h"

#include <libk/idt.h>
#include <libk/internal/idt.h>

/*
 * This global state is a bootloader-only construct and will not be allowed
 * within the actual kernel.
 */

extern struct idt_ptr   __IDTR_DATA;
extern struct idt_entry __IDT_ENTRIES[];
static struct idt_info  idt_info;

struct idt_info *idt_info_init(void)
{
    idt_info = (struct idt_info){
        .idtr           = &__IDTR_DATA,
        .entries        = __IDT_ENTRIES,
        .max_nr_entries = IDT_MAX_ENTRIES,
        .nr_entries     = 0,
    };
    return &idt_info;
}
