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

#include "irq.h"
#include "io.h"
#include "string.h"

/*
 * Everything in this file is extremely specific to the PIC 8259A, so if you
 * want to understand what is going on in this file, you should read the data
 * sheet. If this file were to be extensively documented, it would just be
 * another data sheet. I will, however, provide references to pages.
 */

/* PIC initialisation is global machine state anyway. */

extern struct pic_state_table pic_shadow_table;
struct pic_state_table        pic_state;

void irq_shadow_write(const struct pic_state_table *state)
{
	memcpy(&pic_shadow_table, (void *)state, sizeof(*state));
	return;
}

void irq_init(void)
{
	const struct pic_state_table state = {
	    .pic0_icw1 = ICW1_INIT | ICW1_IC4,
	    .pic1_icw1 = ICW1_INIT | ICW1_IC4,
	    .pic0_icw2 = IRQ0_BASE_PM,
	    .pic1_icw2 = IRQ1_BASE_PM,
	    .pic0_icw3 = IRQ_CASCADE << 1,
	    .pic1_icw3 = IRQ_CASCADE,
	    .pic0_icw4 = ICW4_MODE,
	    .pic1_icw4 = ICW4_MODE,
	};

	/* ICW1 */
	outb(PIC0_CMD, state.pic0_icw1);
	outb(PIC1_CMD, state.pic1_icw1);

	/*
	 * ICW2 - remap IRQs
	 * In protected mode the interval [0x00, 0x20) is reserved
	 */
	outb(PIC0_DATA, state.pic0_icw2);
	outb(PIC1_DATA, state.pic1_icw2);

	/* ICW3, tell the PIC 8259 chips to use master/slave mode */

	outb(PIC0_DATA, state.pic0_icw3);
	outb(PIC1_DATA, state.pic1_icw3);

	/* ICW4, set to 8086 mode */

	outb(PIC0_DATA, state.pic0_icw4);
	outb(PIC1_DATA, state.pic1_icw4);

	/* Write initialised PIC state to tables. */
	pic_state = state;
	irq_shadow_write(&state);

	return;
}

/* Reference: PIC 8259A data sheet p. 13 and p. 17 */
/* Returns a value >0x0F if the read is invalid */

uint16_t irq_read_reg(unsigned char reg)
{
	if(reg != 0x03 && reg != 0x02) return 0x10;

	outb(0x20, 0x08 | reg);
	outb(0xA0, 0x08 | reg);

	return (inb(0xA1) << 8) | inb(0x21);
}

void irq_mask(unsigned char irq)
{
	uint16_t      port;
	unsigned char mask;

	if(irq > 0x0F) return;

	port = irq <= 0x08 ? 0x21 : 0xA1;
	irq  = irq < 0x08 ? irq : irq - 0x08;

	mask = inb(port);
	mask = mask | (1 << irq);
	outb(port, mask);

	return;
}

void irq_unmask(unsigned char irq)
{
	uint16_t      port;
	unsigned char mask;

	if(irq > 0x0F) return;

	port = irq <= 8 ? 0x21 : 0xA1;
	irq  = irq < 8 ? irq : irq - 8;

	mask = inb(port);
	mask = mask & ~(1 << irq);
	outb(port, mask);

	return;
}

void irq_mask_all(void)
{
	outb(0x21, 0xFF);
	outb(0xA1, 0xFF);
}

void irq_unmask_all(void)
{
	outb(0x21, 0x00);
	outb(0xA1, 0x00);

	return;
}
