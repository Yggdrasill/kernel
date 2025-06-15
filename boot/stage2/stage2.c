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

/*
 * stdarg.h is always provided in freestanding environments, and allows us to
 * use the C standard's variable argument lists.
 */

#include <stdarg.h>

#include "string.h"
#include "idt.h"
#include "irq.h"
#include "interrupt.h"
#include "mmap.h"
#include "util.h"

#define PUSH_ARG16(a)                   \
        do {                            \
            __asm__ volatile(           \
                    "push word [%0]"    \
                    : : "m"(a)          \
            );                          \
        } while(0)                      

#define PUSH_ARG32(a)                   \
        do {                            \
            __asm__ volatile(           \
                    "push dword [%0]"   \
                    : : "m"(a)          \
            );                          \
        } while(0)                      

union rmode_ret_t {
    void        *ptr;
    int32_t     i32;
    uint32_t    u32;
};

union rmode_i16 {
    int16_t     i16;
    uint16_t    u16;
};

extern void bios_print(void);
/*
 * rmode_trampoline cannot directly return union because it invokes sret stack
 * behaviour, which then causes rmode_trampoline to pop the wrong value off the
 * stack. This will proceed to cause triple-faulting as a return is made to a
 * bogus address.
 */
extern uint32_t rmode_trampoline(void (*)(void));

union rmode_ret_t rmode_call16(
        struct idt_ptr *idtp, 
        void (*callee)(void), 
        uint16_t argc, 
        ... )
{
    union rmode_ret_t rv;
    uint16_t i;
    uint16_t arg;

    va_list ap;

    va_start(ap, argc);
    for(i = 0; i < argc; i++) {
        arg = va_arg(ap, int);
        PUSH_ARG16(arg);
    }
    va_end(ap);

    ints_flag_clear();
    rv.u32 = rmode_trampoline(callee);
    idt_install(idtp);
    ints_flag_set();

    return rv;
}

int main(struct e820_map *start, struct e820_map *end)
{
#ifdef TEST_MMAP
    struct e820_map test_map[MMAP_MAX_ENTRIES];
    struct e820_map broken_map[] = {
        {0x0, 0x200, 2, 0},
        {0x0, 0x3000, 1, 0},
        {0x3000, 0x7F00, 1, 0},
        {0x3000, 0x200, 2, 0},
        {0x3600, 0x200, 5, 0},
        {0x3600, 0x200, 2, 0},
        {0x7E00, 0x8000, 1, 0},
        {0x30000, 0x200, 1, 0},
        {0x30000, 0x200, 3, 0},
        {0x4F000, 0x2000, 1, 0},
        {0x50000, 0x8000, 3, 0},
        {0x54000, 0x8000, 1, 0},
        {0x57000, 0x1500, 2, 0},
        {0x5a000, 0x1500, 2, 0},
        {0x100000, 0x10000, 2, 0},
        {0xF5000, 0x10000, 1, 0}
    };
#endif
    struct idt_ptr *idtp;
    struct idt_entry *entries;

    entries = (void *)&__IDT_BASE_LOCATION;

    memsetw((int16_t *)&FB_ADDR, 0x0720, 0x7D0);

    puts("Hello world!");

#ifdef TEST_MMAP
    memcpy(test_map, broken_map, sizeof(broken_map) );
    mmap_init(test_map, sizeof(broken_map) / sizeof(*broken_map) );
#else
    mmap_init(start, end - start);
#endif

    idtp = idt_init();
    idt_install(idtp);

    exception_idt_init(entries);

    irq_init();
    irq_idt_init(entries + 0x20);
    irq_mask_all();
    irq_unmask(IRQ_KEYBOARD);
    ints_flag_set();

    for(;;) {
        __asm__ volatile(
                "hlt;"
                );
    }

    __asm__ volatile(
            "cli;"
            "hlt;"
            );
}
