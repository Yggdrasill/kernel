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
extern error

bits    16
section .stage15 alloc exec progbits nowrite

a20_error:
    push  a20_err
    push  a20e_len
    call  error

a20_check:
    push  bp
    mov   bp, sp
    push  ax
    push  di
    push  si
    push  es
    push  ds

    xor   ax, ax
    mov   es, ax
    not   ax
    mov   ds, ax
    mov   di, 0x0500
    mov   si, 0x0510

    mov   ax, word [es:di]
    push  ax
    mov   ax, word [ds:si]
    push  ax

    mov   word [es:di], 0xAA55
    mov   word [ds:si], 0x55AA
    cmp   word [es:di], 0x55AA

    pop   ax
    mov   word [ds:si], ax
    pop   ax
    mov   word [es:di], ax

    pop   ds
    pop   es
    je    a20_end

    mov   [has_a20], byte 0x01
a20_end:
    pop   si
    pop   di
    pop   ax
    pop   bp
    ret

bios_a20:
    push  ax
    mov   ax, 0x2401
    int   0x15
    pop   ax
    ret

kbd8042_wait_cmd:
    in    al, 0x64
    test  al, 2
    jnz   kbd8042_wait_cmd
    ret

kbd8042_wait_data:
    in    al, 0x64
    test  al, 1
    jz    kbd8042_wait_data
    ret

kbd8042_a20:
    push  ax
    cli
    call  kbd8042_wait_cmd
    mov   al, 0xAD
    out   0x64, al

    call  kbd8042_wait_cmd
    mov   al, 0xD0
    out   0x64, al

    call  kbd8042_wait_data
    in    al, 0x60
    push  ax

    call  kbd8042_wait_cmd
    mov   al, 0xD1
    out   0x64, al

    call  kbd8042_wait_cmd
    pop   ax
    or    al, 2
    out   0x60, al

    call  kbd8042_wait_cmd
    mov   al, 0xAE
    out   0x64, al

    call  kbd8042_wait_cmd
    sti
    pop   ax
    ret

a20_ee:
    push  ax
    in    al, 0xEE
    pop   ax
    ret

a20_fast:
    push  ax
    in    al, 0x92
    or    al, 2
    out   0x92, al
    pop   ax
    ret

a20_init:
    push  bp
    mov   bp, sp
    call  a20_check
    cmp   [has_a20], byte 0x01
    je    done_a20

    call  bios_a20
    call  a20_check
    cmp   [has_a20], byte 0x01
    je    done_a20

    call  kbd8042_a20
    call  a20_check
    cmp   [has_a20], byte 0x01
    je    done_a20

    call  a20_ee
    call  a20_check
    cmp   [has_a20], byte 0x01
    je    done_a20

    cmp   [has_a20], byte 0x01
    jne   a20_error
done_a20:
    pop   bp
    ret

section .data
has_a20   db 0

section .rodata
a20_err   db "E: A20 disabled",0x0D,0x0A
a20e_len  equ $ - a20_err

