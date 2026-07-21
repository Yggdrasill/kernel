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

#include <stddef.h>
#include <stdint.h>

struct idt_ptr;
struct idt_entry;
struct idt_info;

extern void idt_install(struct idt_info *);

struct idt_info *idt_init(void);

size_t idt_entries_nr(struct idt_info *);
size_t idt_entries_max(struct idt_info *);

int idt_entry_set(struct idt_info *, void (*)(void), size_t, uint16_t, uint8_t);
int idt_entry_add(struct idt_info *, void (*)(void), uint16_t, uint8_t);

void exception_idt_init(struct idt_info *entries);
void irq_idt_init(struct idt_info *entries);

#endif
