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

global reset
global read
extern __bios_error

%define bios_error __bios_error

bits    16
section .boot.util alloc exec progbits nowrite

reset:
    push  bp
    mov   bp, sp
    push  dx
    mov   dx, [ss:bp + 4]
    push  si
    push  ax
    xor   si, si
resetlp:
    inc   si
    cmp   si, 0x05
    je    reset_e
    xor   word ax, ax
    int   0x13
    jc    resetlp

    pop   word ax
    pop   word si
    pop   dx
    pop   bp
    ret

read:
    push  bp
    mov   word bp, sp
    push  word ax
    push  word bx
    push  word cx
    mov   word bx, [ss:bp + 8]
    mov   word ax, [ss:bp + 6]
    mov   word dx, [ss:bp + 4]
    push  word si
    xor   word si, si
readlp:
    inc   si
    cmp   si, 0x05
    je    read_e
    mov   dh, 0x00
    mov   ch, 0x00
    mov   cl, 0x02
    int   0x13
    jc    readlp

    pop   word si
    pop   word cx
    pop   word bx
    pop   word ax
    pop   bp
    ret

reset_e:
    push  disk_err1
    push  de1_len
    call  bios_error

read_e:
    push  disk_err2
    push  de2_len
    call  bios_error

section .boot.rodata alloc noexec progbits nowrite
disk_err1 db "E: Disk reset failed (5 tries)",0x0D,0x0A
de1_len   equ $ - disk_err1
disk_err2 db "E: Disk read failed (5 tries)",0x0D,0x0A
de2_len   equ $ - disk_err2
