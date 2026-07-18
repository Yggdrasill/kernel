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

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

#include <libk/idt.h>

struct interrupt_info;

extern void nmi_disable(void);
extern void nmi_enable(void);

void exception_idt_init(struct idt_info *entries);
void irq_idt_init(struct idt_info *entries);

void exception_handler(struct interrupt_info *info);
void irq_handler(struct interrupt_info *info);

void ints_flag_clear(void);
void ints_flag_set(void);

uint8_t nmi_status(void);

#endif
