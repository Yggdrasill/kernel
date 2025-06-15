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

#define RMODE_MAX_ARGS 16

#include <stdarg.h>

#include "stdint.h"
#include "string.h"

#include "rmode.h"

#include "idt.h"
#include "mmap.h"
#include "interrupt.h"

void __bios_print(ptr16_t str, uint16_t len);
struct mmap_array *__bios_mmap(void);

/*
 * rmode_trampoline cannot directly return union because it invokes sret stack
 * behaviour, which then causes rmode_trampoline to pop the wrong value off the
 * stack. This will proceed to cause triple-faulting as a return is made to a
 * bogus address.
 */

extern uint32_t rmode_trampoline(void (*)(void));

#define PUSH_ARG16(arg)             \
    do {                            \
        __asm__ volatile(           \
                "push word ptr %0"  \
                :                   \
                : "m"(arg)          \
                : "memory"          \
        );                          \
    } while(0);                     

#define PUSH_ARG32(arg)             \
    do {                            \
        __asm__ volatile(           \
                "push dword ptr %0" \
                :                   \
                : "m"(arg)          \
                : "memory"          \
        );                          \
    } while(0);                     

#define RMODE_CALL(rv)                  \
    do {                                \
        __asm__ volatile(               \
                "call rmode_trampoline" \
                : "=a"(rv->u32)         \
                :                       \
                : "memory"              \
    );                                  \
    } while(0);

#define ARGS_CLEANUP(n_args, size)      \
    do {                                \
        __asm__ volatile(               \
                "add esp, %0"           \
                :                       \
                : "r"(n_args * size)    \
                : "memory"              \
        );                              \
    } while(0);

void rmode_call16(
        union rmode_ret_t *rv,
        struct idt_ptr *idtp, 
        void (*callee)(void), 
        uint16_t argc, 
        ... )
{
    uint16_t args[RMODE_MAX_ARGS];
    uint16_t i;

    va_list ap;

    if(argc > RMODE_MAX_ARGS) {
        puts("args > 16, maximum real mode arguments exceeded!");
        rv->i32 = -1;
        goto exit16;
    }

    /*
     * Can't manipulate the stack while va_list is used.
     */

    va_start(ap, argc);
    for(i = 0; i < argc; i++) {
        args[i] = (uint16_t)va_arg(ap, int);
    }
    va_end(ap);

    ints_flag_clear();

    /*
     * This is a manual function call made with __asm__ inlines. We first push
     * all the arguments in the arguments array, which were provided by va_list
     * before. We then push the callee's function pointer, call rmode_trampline,
     * and finally clean up all arguments manually.
     */

    for(i = 0; i < argc; i++) {
        PUSH_ARG16(args[i]);
    }
    PUSH_ARG32(callee);
    RMODE_CALL(rv); 
    ARGS_CLEANUP(1, sizeof(callee) );
    ARGS_CLEANUP(argc, sizeof(argc) );

    idt_install(idtp);
    ints_flag_set();

exit16:
    return;
}

void rmode_call32(
        union rmode_ret_t *rv,
        struct idt_ptr *idtp, 
        void (*callee)(void), 
        uint32_t argc, 
        ... )
{
    uint32_t args[RMODE_MAX_ARGS];
    uint32_t i;

    va_list ap;

    /*
     * Can't manipulate the stack while va_list is used.
     */

    if(argc > RMODE_MAX_ARGS) {
        puts("args > 16, maximum real mode arguments exceeded!");
        rv->i32 = -1;
        goto exit32;
    }

    va_start(ap, argc);
    for(i = 0; i < argc; i++) {
        args[i] = va_arg(ap, int);
    }
    va_end(ap);

    ints_flag_clear();

    /*
     * This is a manual function call made with __asm__ inlines. We first push
     * all the arguments in the arguments array, which were provided by va_list
     * before. We then push the callee's function pointer, call rmode_trampline,
     * and finally clean up all arguments manually.
     */

    for(i = 0; i < argc; i++) {
        PUSH_ARG32(args[i]);
    }
    PUSH_ARG32(callee);
    RMODE_CALL(rv); 
    ARGS_CLEANUP(1, sizeof(callee) );
    ARGS_CLEANUP(argc, sizeof(argc) );

    idt_install(idtp);
    ints_flag_set();

exit32:
    return;
}

struct mmap_array *bios_mmap(struct idt_ptr *idtp)
{
    union rmode_ret_t rv;

    rmode_call16(&rv, idtp, (void (*)(void))__bios_mmap, 0);

    return rv.ptr;
}

