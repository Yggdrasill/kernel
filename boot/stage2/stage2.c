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

#include "mmap.h"
#include "pmm.h"
#include "rmode.h"
#include "stage2.h"
#include "vga.h"

#include <boot/common/disk.h>

#include <libk/gdt.h>
#include <libk/idt.h>
#include <libk/interrupt.h>
#include <libk/irq.h>
#include <libk/mm.h>
#include <libk/mmap.h>
#include <libk/util.h>

#define BUFFER_SIZE (64 * (1 << 10))

/* Preallocated bootstrap storage from boot/common/linker.lds.S. */

extern struct gdt_ptr __GDTR_DATA;
extern struct idt_ptr __IDTR_DATA;

extern struct gdt_entry __GDT_ENTRIES[GDT_MAX_ENTRIES];
extern struct idt_entry __IDT_ENTRIES[IDT_MAX_ENTRIES];

extern struct e820_map __mmap_old_map[MMAP_MAX_ENTRIES];
extern struct e820_map __mmap_new_map[MMAP_MAX_ENTRIES];

extern int16_t __framebuffer[];

static struct gdt_info  gdt_info;
static struct idt_info  idt_info;
static struct e820_info old_mmap_info;
static struct e820_info new_mmap_info;

struct boot_info {
    struct gdt_info  *gdt;
    struct idt_info  *idt;
    struct e820_info *mmap;

    char  *buffer;
    size_t size;
};

static struct boot_info boot_init(void)
{
    char *buffer;

    if(irq_read_imr() != 0xFFFF) irq_mask_all();
    if(!nmi_status()) nmi_disable();

    fb_init(__framebuffer);

    gdt_info = (struct gdt_info){
        .gdtr           = &__GDTR_DATA,
        .entries        = __GDT_ENTRIES,
        .nr_entries     = 0,
        .max_nr_entries = GDT_MAX_ENTRIES,
    };

    gdt_init(&gdt_info);

    idt_info = (struct idt_info){
        .idtr           = &__IDTR_DATA,
        .entries        = __IDT_ENTRIES,
        .nr_entries     = 0,
        .max_nr_entries = IDT_MAX_ENTRIES,
    };

    idt_init(&idt_info);
    exception_idt_init(&idt_info);
    irq_idt_init(&idt_info);

    irq_init();
    irq_mask_all();
    nmi_enable();
    ints_flag_set();

    old_mmap_info = (struct e820_info){
        .base           = __mmap_old_map,
        .nr_entries     = 0,
        .max_nr_entries = MMAP_MAX_ENTRIES,
    };

    new_mmap_info = (struct e820_info){
        .base           = __mmap_new_map,
        .nr_entries     = 0,
        .max_nr_entries = MMAP_MAX_ENTRIES,
    };

    /* Panics if error */
    bios_mmap(&old_mmap_info);
    boot_mmap_init(&new_mmap_info, &old_mmap_info);

    pmm_init(&new_mmap_info);
    buffer = pmm_alloc_range(BUFFER_SIZE, 0x0, 0x100000);

    return (struct boot_info){
        .gdt    = &gdt_info,
        .idt    = &idt_info,
        .mmap   = &new_mmap_info,
        .buffer = buffer,
        .size   = BUFFER_SIZE,
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
    struct disk_info disk;

    info = boot_init();
    irq_unmask(IRQ_NUM_KBD);
    bios_disk_geometry(&disk, 0xFF);
    bios_disk_geometry(&disk, 0x80);
    bios_disk_reset(0x80);

    halt();
    hcf();

    return 0;
}
