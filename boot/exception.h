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

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "idt.h"
#include "stdint.h"

struct exception_info {
  uint32_t fs, gs, es, ds;
  uint32_t edi, esi, ebp, esp;
  uint32_t ebx, edx, ecx, eax;
  uint32_t intno, errno;
  uint32_t eip, cs, eflags, prev_esp, ss;
};

extern void exception_unknown(void);
extern void exception_0x00(void);
extern void exception_0x01(void);
extern void exception_0x02(void);
extern void exception_0x03(void);
extern void exception_0x04(void);
extern void exception_0x05(void);
extern void exception_0x06(void);
extern void exception_0x07(void);
extern void exception_0x08(void);
extern void exception_0x0A(void);
extern void exception_0x0B(void);
extern void exception_0x0C(void);
extern void exception_0x0D(void);

void exceptions_init(struct idt_entry *);
void exception_handler(struct exception_info *);

#endif
