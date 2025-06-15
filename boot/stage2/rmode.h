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

#ifndef RMODE_H
#define RMODE_H

#include "stdint.h"
#include "idt.h"
#include "mmap.h"

union rmode_ret_t {
    void        *ptr;
    int32_t     i32;
    uint32_t    u32;
};

union rmode_i16 {
    int16_t     i16;
    uint16_t    u16;
};


void rmode_call16(union rmode_ret_t *, struct idt_ptr *, void (*)(void), uint16_t argc, ...);
void rmode_call32(union rmode_ret_t *, struct idt_ptr *, void (*)(void), uint32_t argc, ...);

struct mmap_array *bios_mmap(struct idt_ptr *);

#endif
