; MBR bootloader, currently unnamed
; Copyright (C) 2017  Yggdrasill <kaymeerah@lambda.is>

; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.

; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

global a20_init
extern __bios_error

bits    16
section .stage15 alloc exec progbits nowrite

a20_check:
    push   byte -1
    pop    es
    mov    si, 0x0500
    mov    di, 0x0510

    mov    al, [es:di]
    mov    ah, [ds:si]

    mov    byte [es:di], 0xAA
    mov    byte [ds:si], 0x55
    ; Do NOT alter flags after this!
    cmp    byte [es:di], 0x55

    mov    [ds:si], ah
    mov    [es:di], al

    je     check_exit
    pop    ax
check_exit:
    ret 

kbd8042_wait_cmd:
    in     al, dx
    test   al, 2
    jnz    kbd8042_wait_cmd
    ret 

kbd8042_wait_data:
    in     al, dx
    test   al, 1
    jz     kbd8042_wait_data
    ret 

; Inlined everything, for space. Doesn't
; return, instead returns are handled by
; a20_check. That is, a20_check pops the
; return pointer and rets into the caller.
a20_init:
    call   a20_check

    ; BIOS A20.
    sti
    mov    ax, 0x2401
    int    0x15
    cli
    call   a20_check

    ; Keyboard A20. If the keyboard
    ; controller doesn't respond in
    ; 100ms, assume it's problematic.
    ; If it does, don't check again.

    ; Wait for a somewhat approximate
    ; ~100ms. Because the timer doesn't
    ; latch, it can be inaccurate.
    mov    dx, 0x64
    mov    cx, 468
kbd8042_cmd_try:
    ; Initial timer state.
    in     al, 0x42
timer_count:
    ; Wait for the timer.
    xchg   al, ah
    in     al, 0x42
    cmp    al, ah
    jbe    timer_count
    ; Poll input buffer status.
    in     al, dx
    test   al, 2
    loopnz kbd8042_cmd_try
    jnz    a20_next

kbd8042_next:
    mov    al, 0xAD
    out    dx, al
    call   kbd8042_wait_cmd

    mov    al, 0xA7
    out    dx, al
    call   kbd8042_wait_cmd

    in     al, dx
    test   al, 1
    jz     kbd8042_continue
    in     al, 0x60

kbd8042_continue:
    mov    al, 0xD0
    out    dx, al
    call   kbd8042_wait_data

    in     al, 0x60
    push   ax
    call   kbd8042_wait_cmd

    mov    al, 0xD1
    out    dx, al
    call   kbd8042_wait_cmd

    pop    ax
    or     al, 2
    out    0x60, al
    call   kbd8042_wait_cmd

    mov    al, 0xAE
    out    dx, al
    call   kbd8042_wait_cmd
    call   a20_check

a20_next:
    ; Read 0xEE A20.
    in     al, 0xEE
    call   a20_check

    ; Fast A20.
    in     al, 0x92
    and    al, 0xFE
    or     al, 2
    out    0x92, al
    call   a20_check
a20_error:
    push   dword a20e_len
    push   dword a20_err
    call   __bios_error

section .stage15.rodata
a20_err  db "E: A20 disabled",0x0D,0x0A
a20e_len equ $ - a20_err
