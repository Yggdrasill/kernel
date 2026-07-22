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

#include <string.h>

#include "gdt.h"
#include "idt.h"
#include "mmap.h"
#include "pmm.h"
#include "rmode.h"

#include <libk/gdt.h>
#include <libk/idt.h>
#include <libk/interrupt.h>
#include <libk/irq.h>
#include <libk/mmap.h>
#include <libk/util.h>

struct boot_info {
    struct gdt_info  *gdt;
    struct idt_info  *idt;
    struct e820_info *mmap;
};

static struct boot_info boot_init(void)
{
    struct gdt_info  *gdt;
    struct idt_info  *idt;
    struct e820_info *mmap;

    if(irq_read_imr() != 0xFFFF) irq_mask_all();
    if(!nmi_status()) nmi_disable();

    gdt = gdt_info_init();
    gdt_init(gdt);

    memsetw((int16_t *)&FB_ADDR, 0x0720, 0x7D0);

    idt = idt_info_init();
    idt_init(idt);
    exception_idt_init(idt);
    irq_idt_init(idt);

    irq_init();
    irq_mask_all();
    nmi_enable();
    ints_flag_set();

    mmap = boot_mmap_init();
    /* Panics if error */
    bios_mmap(mmap);
    mmap = boot_mmap_setup(mmap);

    return (struct boot_info){
        .gdt  = gdt,
        .idt  = idt,
        .mmap = mmap,
    };
}

int main(void)
{
#if 0
    /* Preserved for later test writing */
    struct e820_map test_map[MMAP_MAX_ENTRIES];
    struct e820_map broken_map[] = {
        {0x0,      0x200,   2, 0},
        {0x0,      0x3000,  1, 0},
        {0x3000,   0x7F00,  1, 0},
        {0x3000,   0x200,   2, 0},
        {0x3600,   0x200,   5, 0},
        {0x3600,   0x200,   2, 0},
        {0x7E00,   0x8000,  1, 0},
        {0x30000,  0x200,   1, 0},
        {0x30000,  0x200,   3, 0},
        {0x4F000,  0x2000,  1, 0},
        {0x50000,  0x8000,  3, 0},
        {0x54000,  0x8000,  1, 0},
        {0x57000,  0x1500,  2, 0},
        {0x5a000,  0x1500,  2, 0},
        {0x100000, 0x10000, 2, 0},
        {0x105000, 0x10000, 7, 0},
        {0xF5000,  0x10000, 1, 0}
    };
#endif
    struct boot_info info;

    info = boot_init();
    irq_unmask(IRQ_NUM_KBD);
    pmm_init(info.mmap);

    halt();
    hcf();

    return 0;
}
