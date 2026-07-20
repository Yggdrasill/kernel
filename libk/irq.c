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

#include <libk/io.h>
#include <libk/irq.h>

/*
 * Everything in this file is extremely specific to the PIC 8259A, so if you
 * want to understand what is going on in this file, you should read the data
 * sheet. If this file were to be extensively documented, it would just be
 * another data sheet. I will, however, provide references to pages.
 */

/*
 * This may seem strange, but there is a fortunate reality to the interrupt
 * vectors. IBM published a document in April 1987 titled:
 * PS/2 and PC BIOS Interface Technical Reference Apr87
 *
 * This document defines interrupt vectors [0x20, 0x40) as reserved for DOS. In
 * practice this means that any IBM compatible machine must adhere to this, as
 * MS-DOS made extensive use of these interrupt vectors for its own purposes.
 *
 * Since this is not an MS-DOS environment, and since any IBM-compatible that
 * intends to run MS-DOS cannot use these vectors, it should in theory be fine
 * to alias the interrupt vectors. This helps rmode_trampoline later, where the
 * PICs do not need to be reinitialised for real mode execution.
 */

void irq_ivt_alias(uint8_t pic0_base, uint8_t pic1_base)
{
    uint32_t *pic0_vectors;
    uint32_t *pic1_vectors;
    uint32_t *pic0_shadow;
    uint32_t *pic1_shadow;
    pic0_vectors = (uint32_t *)(IVT_VECTOR_SIZE * IRQ0_BASE_RM);
    pic1_vectors = (uint32_t *)(IVT_VECTOR_SIZE * IRQ1_BASE_RM);
    pic0_shadow  = (uint32_t *)(IVT_VECTOR_SIZE * pic0_base);
    pic1_shadow  = (uint32_t *)(IVT_VECTOR_SIZE * pic1_base);
    memcpy(pic0_shadow, pic0_vectors, IVT_VECTOR_SIZE * NR_PIC_IRQS);
    memcpy(pic1_shadow, pic1_vectors, IVT_VECTOR_SIZE * NR_PIC_IRQS);
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
    irq_ivt_alias(IRQ0_BASE_PM, IRQ1_BASE_PM);

    return;
}

/*
 * Reference: PIC 8259A data sheet p. 13 and p. 14.
 * Reading the IMR can be done by OCW1.
 */

uint16_t irq_read_imr(void)
{
    return (inb(0xA1) << 8) | inb(0x21);
}

/*
 * Reference: PIC 8259A data sheet p. 13 and p. 17.
 * The following code is OCW3 to read the IRR or ISR.
 * - reg = 0x02
 *   Read the interrupt request register.
 * - reg = 0x03
 *   Read the interrupt service register.
 * Precondition: reg == 0x02 || reg == 0x03
 */

uint16_t irq_read_reg(unsigned char reg)
{
    outb(0x20, 0x08 | reg);
    outb(0xA0, 0x08 | reg);
    return (inb(0xA0) << 8) | inb(0x20);
}

void irq_mask(unsigned char irq)
{
    uint16_t      port;
    unsigned char mask;

    if(irq > 0x0F) return;

    port = irq < 0x08 ? 0x21 : 0xA1;
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

    port = irq < 8 ? 0x21 : 0xA1;
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
