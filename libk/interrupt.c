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

#include <libk/idt.h>
#include <libk/interrupt.h>
#include <libk/io.h>
#include <libk/irq.h>
#include <libk/util.h>

struct interrupt_info {
    uint32_t fs, gs, es, ds;
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t intno, errno;
    uint32_t eip, cs, eflags, prev_esp, ss;
};

extern uint8_t shadow_p70;

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
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack segment fault",
    "General protection fault",
    "Page fault",
    "Unhandled exception!",
    "Floating point exception",
    "Alignment check",
    "Machine check",
    "SIMD floating point exception",
    "Virtualisation exception",
    "Control protection exception",
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
    "IRQ 15",
};

void exception_handler(struct interrupt_info *info)
{
    if(info->intno > 0x1F) return;

    if(info->intno == 0x1F) puts("Unhandled exception!");
    else puts(exceptions[info->intno]);

    hcf();
}

void irq_handler(struct interrupt_info *info)
{
    uint16_t spurious;
    spurious = !(irq_read_reg(0x03) & (1 << info->intno));
    if(spurious) goto eoi;

    switch(info->intno) {
        case IRQ_NUM_KBD: port_read_byte(0x60);
    }

    puts(irq_interrupts[info->intno]);

    /*
     * OCW2 EOI
     * Filter out spurious interrupts from EOI.
     */
eoi:
    if(!spurious && info->intno >= 0x08) port_write_byte(0xA0, 0x20);
    if(!spurious || info->intno >= 0x08) port_write_byte(0x20, 0x20);

    return;
}

uint8_t nmi_status(void)
{
    return shadow_p70 & 0x80;
}
