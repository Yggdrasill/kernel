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

#include "io.h"
#include "string.h"
#include "interrupt.h"

char *exceptions[] = {
    "Division by zero",
    "Debug interrupt",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault"
};

char *irq_interrupts[] = {
    "Timer",
    "Keyboard",
    "Cascade",
    "Serial port 2",
    "Serial port 1",
    "Parallel port 2",
    "Diskette",
    "Parallel port 1",
    "CMOS RTC",
    "CGA retrace",
    "IRQ 10",
    "IRQ 11",
    "Auxiliary",
    "FPU",
    "Hard disk",
    "IRQ 15"
};

/* This is disgusting, I know, but also necessary */

void exception_idt_init(struct idt_entry *entries)
{
    idt_set_entry(entries++, &exception_0x00, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x01, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x02, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x03, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x04, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x05, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x06, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x07, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x08, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_unknown, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x0A, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x0B, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x0C, 0x08, 0x8E);
    idt_set_entry(entries++, &exception_0x0D, 0x08, 0x8E);

    /* while(entries < 0x20) */

    while(entries != (&__IDT_BASE_LOCATION) + IDT_ENTRY_NUM) {
        idt_set_entry(entries++, &exception_unknown, 0x08, 0x8E);
    }

    return;
}

void irq_idt_init(struct idt_entry *entries)
{
    idt_set_entry(entries++, &irq_0x00, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x01, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x02, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x03, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x04, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x05, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x06, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x07, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x08, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x09, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x0A, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x0B, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x0C, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x0D, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x0E, 0x08, 0x8E);
    idt_set_entry(entries++, &irq_0x0F, 0x08, 0x8E);

    return;
}

void exception_handler(struct interrupt_info *info)
{
    if(info->intno > 0x1F) return;

    if(info->intno == 0x1F) puts("Unhandled exception!");
    else puts(exceptions[info->intno]);

    __asm__ volatile(
            "hlt;"
            );
}

void irq_handler(struct interrupt_info *info)
{
    char ch;

    switch(info->intno) {
        case IRQ_KEYBOARD:
            ch = inb(0x60);
    }

    outb(0x20, 0x20);
    if(info->intno > 0x08) {
        outb(0xA0, 0x20);
    }

    puts(irq_interrupts[info->intno]);

    return;
}
