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
#include <libk/io.h>
#include <libk/irq.h>
#include <libk/interrupt.h>

struct interrupt_info {
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
extern void exception_0x09(void);
extern void exception_0x0A(void);
extern void exception_0x0B(void);
extern void exception_0x0C(void);
extern void exception_0x0D(void);
extern void exception_0x0E(void);
extern void exception_0x10(void);
extern void exception_0x11(void);
extern void exception_0x12(void);
extern void exception_0x13(void);
extern void exception_0x14(void);
extern void exception_0x15(void);

extern void irq_0x00(void);
extern void irq_0x01(void);
extern void irq_0x02(void);
extern void irq_0x03(void);
extern void irq_0x04(void);
extern void irq_0x05(void);
extern void irq_0x06(void);
extern void irq_0x07(void);
extern void irq_0x08(void);
extern void irq_0x09(void);
extern void irq_0x0A(void);
extern void irq_0x0B(void);
extern void irq_0x0C(void);
extern void irq_0x0D(void);
extern void irq_0x0E(void);
extern void irq_0x0F(void);
extern void irq_wrapper(void);

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

/* This is disgusting, I know, but also necessary */

void exception_idt_init(struct idt_info *info)
{
	size_t entries;

	idt_entry_set(info, &exception_0x00, 0x08, 0x8E, 0x00);
	idt_entry_set(info, &exception_0x01, 0x08, 0x8E, 0x01);
	idt_entry_set(info, &exception_0x02, 0x08, 0x8E, 0x02);
	idt_entry_set(info, &exception_0x03, 0x08, 0x8E, 0x03);
	idt_entry_set(info, &exception_0x04, 0x08, 0x8E, 0x04);
	idt_entry_set(info, &exception_0x05, 0x08, 0x8E, 0x05);
	idt_entry_set(info, &exception_0x06, 0x08, 0x8E, 0x06);
	idt_entry_set(info, &exception_0x07, 0x08, 0x8E, 0x07);
	idt_entry_set(info, &exception_0x08, 0x08, 0x8E, 0x08);
	idt_entry_set(info, &exception_0x09, 0x08, 0x8E, 0x09);
	idt_entry_set(info, &exception_0x0A, 0x08, 0x8E, 0x0A);
	idt_entry_set(info, &exception_0x0B, 0x08, 0x8E, 0x0B);
	idt_entry_set(info, &exception_0x0C, 0x08, 0x8E, 0x0C);
	idt_entry_set(info, &exception_0x0D, 0x08, 0x8E, 0x0D);
	idt_entry_set(info, &exception_0x0E, 0x08, 0x8E, 0x0E);
	idt_entry_set(info, &exception_unknown, 0x08, 0x8E, 0x0F);
	idt_entry_set(info, &exception_0x10, 0x08, 0x8E, 0x10);
	idt_entry_set(info, &exception_0x11, 0x08, 0x8E, 0x11);
	idt_entry_set(info, &exception_0x12, 0x08, 0x8E, 0x12);
	idt_entry_set(info, &exception_0x13, 0x08, 0x8E, 0x13);
	idt_entry_set(info, &exception_0x14, 0x08, 0x8E, 0x14);
	idt_entry_set(info, &exception_0x15, 0x08, 0x8E, 0x15);

	do {
		entries = idt_entry_add(info, &exception_unknown, 0x08, 0x8E);
	} while(entries < idt_entries_max(info));

	return;
}

void irq_idt_init(struct idt_info *info)
{
	idt_entry_set(info, &irq_0x00, 0x08, 0x8E, IRQ0_BASE_PM);
	idt_entry_set(info, &irq_0x01, 0x08, 0x8E, IRQ0_BASE_PM + 0x1);
	idt_entry_set(info, &irq_0x02, 0x08, 0x8E, IRQ0_BASE_PM + 0x2);
	idt_entry_set(info, &irq_0x03, 0x08, 0x8E, IRQ0_BASE_PM + 0x3);
	idt_entry_set(info, &irq_0x04, 0x08, 0x8E, IRQ0_BASE_PM + 0x4);
	idt_entry_set(info, &irq_0x05, 0x08, 0x8E, IRQ0_BASE_PM + 0x5);
	idt_entry_set(info, &irq_0x06, 0x08, 0x8E, IRQ0_BASE_PM + 0x6);
	idt_entry_set(info, &irq_0x07, 0x08, 0x8E, IRQ0_BASE_PM + 0x7);

	idt_entry_set(info, &irq_0x08, 0x08, 0x8E, IRQ1_BASE_PM);
	idt_entry_set(info, &irq_0x09, 0x08, 0x8E, IRQ1_BASE_PM + 0x1);
	idt_entry_set(info, &irq_0x0A, 0x08, 0x8E, IRQ1_BASE_PM + 0x2);
	idt_entry_set(info, &irq_0x0B, 0x08, 0x8E, IRQ1_BASE_PM + 0x3);
	idt_entry_set(info, &irq_0x0C, 0x08, 0x8E, IRQ1_BASE_PM + 0x4);
	idt_entry_set(info, &irq_0x0D, 0x08, 0x8E, IRQ1_BASE_PM + 0x5);
	idt_entry_set(info, &irq_0x0E, 0x08, 0x8E, IRQ1_BASE_PM + 0x6);
	idt_entry_set(info, &irq_0x0F, 0x08, 0x8E, IRQ1_BASE_PM + 0x7);

	return;
}

void exception_handler(struct interrupt_info *info)
{
	if(info->intno > 0x1F) return;

	if(info->intno == 0x1F) puts("Unhandled exception!");
	else puts(exceptions[info->intno]);

	__asm__ volatile("hlt;");
}

void irq_handler(struct interrupt_info *info)
{
	uint16_t spurious;
	spurious = !(irq_read_reg(0x03) & (1 << info->intno));
	if(spurious) goto eoi;

	switch(info->intno) {
		case IRQ_NUM_KBD: inb(0x60);
	}

	puts(irq_interrupts[info->intno]);

	/*
	 * OCW2 EOI
	 * Filter out spurious interrupts from EOI.
	 */
eoi:
	if(!spurious && info->intno >= 0x08) outb(0xA0, 0x20);
	if(!spurious || info->intno >= 0x08) outb(0x20, 0x20);

	return;
}

void ints_flag_clear(void)
{
	__asm__ volatile("cli;");

	return;
}

void ints_flag_set(void)
{
	__asm__ volatile("sti;");

	return;
}

uint8_t nmi_status(void)
{
	return shadow_p70 & 0x80;
}
