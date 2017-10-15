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

#include "string.h"
#include "idt.h"
#include "irq.h"
#include "interrupt.h"

/* PLEASE read the README provided in the same directory. */

int main(void)
{
  struct idt_ptr *idtr;
  struct idt_entry *entries;

  entries = (void *)IDT_BASE_OFFSET;

  clear();

  puts("Hello world!");

  idtr = idt_init();
  idt_install(idtr);

  exception_idt_init(entries);

  irq_init();
  irq_idt_init(entries + 0x20);
  irq_mask_all();
  irq_unmask(IRQ_KEYBOARD);

  __asm__ volatile(
    "sti;"
  );

  for(;;) {
    __asm__ volatile(
      "hlt;"
    );
  }

  __asm__ volatile(
    "cli;"
    "hlt;"
  );
}
