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

#ifndef IDT_H
#define IDT_H

#include "stdint.h"

struct idt_ptr;
struct idt_entry;
struct idt_info;

struct idt_info *idt_init(void);
size_t idt_num_entries(struct idt_info *);
size_t idt_max_entries(struct idt_info *);
size_t idt_add_entry(struct idt_info *, void (*)(void), uint16_t, unsigned char);
void idt_install(struct idt_ptr *);

#endif
