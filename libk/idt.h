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

#define IDT_ENTRY_NUM     256

/*
 * Little endian byte alignment follows. These structures are manually packed
 * because it avoids using compiler pragmas. I do not know of any x86 compiler
 * that would pad a struct that contains only byte-aligned types.
 *
 * Both of these structs will be 8-byte aligned manually.
 */

struct idt_ptr {
    unsigned char limit_0;
    unsigned char limit_8;
    unsigned char base_0;
    unsigned char base_8;
    unsigned char base_16;
    unsigned char base_24;
};

struct idt_entry {
    unsigned char offset_0;
    unsigned char offset_8;
    unsigned char selector_0;
    unsigned char selector_8;
    unsigned char zero;
    unsigned char flags;
    unsigned char offset_16;
    unsigned char offset_24;
};

extern struct idt_ptr __IDT_PTR_LOCATION;
extern struct idt_entry __IDT_BASE_LOCATION;

struct idt_ptr *idt_init(void);
void idt_set_entry(struct idt_entry *,
        void (*)(void),
        uint16_t,
        unsigned char);
void idt_install(struct idt_ptr *);


#endif
