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

#ifndef IRQ_H
#define IRQ_H

#include "idt.h"
#include "interrupt.h"

#define PIC0_CMD  0x20
#define PIC0_DATA 0x21
#define PIC1_CMD  0xA0
#define PIC1_DATA 0xA1

/* IRQ0_BASEx_PM **MUST** be in range [0x20, 0x40). */

#define IVT_VECTOR_SIZE 4
#define NR_PIC_IRQS     8
#define IRQ0_BASE_RM    0x08
#define IRQ1_BASE_RM    0x70
#define IRQ0_BASE_PM    0x20
#define IRQ1_BASE_PM    IRQ0_BASE_PM + 8
#define IRQ_CASCADE     0x02

enum IRQ_NUMS {
	IRQ_NUM_PIT = 0,
	IRQ_NUM_KBD = 1,
	IRQ_NUM_CASC,
	IRQ_NUM_COM2,
	IRQ_NUM_COM1,
	IRQ_NUM_LPT2,
	IRQ_NUM_FLOP,
	IRQ_NUM_LPT1,
	IRQ_NUM_CMOS,
	IRQ_NUM_RES1,
	IRQ_NUM_RES2,
	IRQ_NUM_RES3,
	IRQ_NUM_PS2M,
	IRQ_NUM_FPU,
	IRQ_NUM_ATA1,
	IRQ_NUM_ATA2,
};

/*
 * Reference: PIC8259A datasheet p. 11
 * IC4  low = no ICW4,      high = expect ICW4
 * SNGL low = cascade mode, high = single mode
 * ADI  low = interval 8,   high = interval 4
 * LTIM low = edge trigger, high = level trigger
 * INIT low = not ICW1,     high = command ICW1
 * bits 5-7 inclusive reserved for MCS 8085
 *
 * Specifically for x86:
 *  - ADI CALL interval should be 8, and is ignored on x86
 *  - SNGL should be low, as all IBM AT compatibles have cascaded PICs
 * Specific considerations:
 *  - LTIM should be low, as edge triggers prevents interrupt blocking
 */

enum PIC_8259A_ICW1_BITS {
	ICW1_IC4  = 1,
	ICW1_SNGL = 1 << 1,
	ICW1_ADI  = 1 << 2,
	ICW1_LTIM = 1 << 3,
	ICW1_INIT = 1 << 4,
	ICW1_RES0 = 0 << 5,
	ICW1_RES1 = 0 << 6,
	ICW1_RES2 = 0 << 7,
};

/*
 * Reference: PIC8529A datasheet p. 12
 * MODE low = MCS 8085 mode, high = 8086 mode
 * AEOI low = auto EOI,      high = explicit EOI
 * BUFS low = PIC1 buffered, high = PIC0 buffered
 * BUF  low = buffer enable, high = disable any buffering
 * SFNM low = no nesting,    high = enable special fully nested mode
 *
 * NOTE: BUF disables PIC0/1 buffering altogether.
 *
 * Specifically for x86:
 *  - MODE should be set high
 * Specific considerations:
 *  - Buffering, nesting, and auto EOI are all undesired.
 */

enum PIC_8259A_ICW4_BITS {
	ICW4_MODE = 1,
	ICW4_AEOI = 1 << 1,
	ICW4_BUFS = 1 << 2,
	ICW4_BUF  = 1 << 3,
	ICW4_SFNM = 1 << 4,
	ICW4_RES0 = 0 << 5,
	ICW4_RES1 = 0 << 6,
	ICW4_RES2 = 0 << 7,
};

struct pic_state_table {
	uint8_t pic0_icw1;
	uint8_t pic1_icw1;
	uint8_t pic0_icw2;
	uint8_t pic1_icw2;
	uint8_t pic0_icw3;
	uint8_t pic1_icw3;
	uint8_t pic0_icw4;
	uint8_t pic1_icw4;
};

void     irq_init(void);
uint16_t irq_read_imr(void);
uint16_t irq_read_reg(unsigned char);
void     irq_unmask(unsigned char);
void     irq_unmask_all(void);
void     irq_mask_all(void);

#endif
