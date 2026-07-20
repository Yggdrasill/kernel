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

extern __STACK_BASE_LOCATION

extern __bios_error

section ._init alloc exec progbits nowrite
global __start

__start:
bits 16
    xor    ax, ax
    inc    ax
    shl    ax, 16
    bt     ax, 16
    jc     continue_32
    push   dword exec_len
    push   dword exec_err
    call   __bios_error
bits 32
continue_32:
    push  dword 0x02
    popfd

    xor   ebp, ebp
    mov   esp, __STACK_BASE_LOCATION
    call  main
    ; if we ever get to this point, 
    ; just give up, something went
    ; wrong.
    cli
hltlp:
    hlt
    jmp   hltlp

exec_err db "E: Cannot execute stage 2 in 16-bit mode!",0x0D,0x0A
exec_len equ $ - exec_err

section .text
extern main
