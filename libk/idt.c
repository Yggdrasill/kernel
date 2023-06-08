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
#include "string.h"
#include "stdint.h"

extern struct idt_ptr idtr __attribute__((section("idt")));

struct idt_ptr *idt_init(void)
{
  struct idt_ptr *idtp;
  unsigned char *arr_limit;
  unsigned char *arr_base;

  uint32_t base;
  uint16_t limit;

  idtp = &idtr;

  limit = sizeof(struct idt_entry) * IDT_ENTRY_NUM - 1;
  puthex(limit);
  puthex((size_t)idtp);
  base = (uint32_t)IDT_BASE_OFFSET;

  arr_limit = (unsigned char *)&limit;
  arr_base = (unsigned char *)&base;

  idtp->limit_0 = arr_limit[0];
  idtp->limit_8 = arr_limit[1];

  idtp->base_0 = arr_base[0];
  idtp->base_8 = arr_base[1];
  idtp->base_16 = arr_base[2];
  idtp->base_24 = arr_base[3];

  return idtp;
}

void idt_set_entry(struct idt_entry *entry,
                   void (*idt_handler)(void),
                   uint16_t select,
                   unsigned char flags)
{
  intptr_t raw_ptr;
  unsigned char *offset;
  unsigned char *selector;

  raw_ptr = (intptr_t)idt_handler;
  offset = (unsigned char *)&raw_ptr;
  selector = (unsigned char *)&select;

  entry->offset_0 = offset[0];
  entry->offset_8 = offset[1];
  entry->offset_16 = offset[2];
  entry->offset_24 = offset[3];

  entry->selector_0 = selector[0];
  entry->selector_8 = selector[1];

  entry->zero = 0;
  entry->flags = flags;

  return;
}

void idt_install(struct idt_ptr *idtr)
{
  __asm__ volatile(
    "mov  eax, %0;"
    "lidt [eax];" : : "m"(idtr)
  );

  return;
}
