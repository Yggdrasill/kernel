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

bits 32
section .text

global port_io_write
global port_io_read
global nmi_enable
global nmi_disable
global gdt_install
global idt_install
global get_stack_base
global hcf
global halt
global ints_flag_clear
global ints_flag_set

extern shadow_p70

port_write_byte:
    out  dx, al
    jmp  short port_io_return

port_write_word:
    out  dx, ax
    jmp  short port_io_return

port_write_dword:
    out  dx, eax
    jmp  short port_io_return

port_read_byte:
    in   al, dx
    jmp  short port_io_return

port_read_word:
    in   ax, dx
    jmp  short port_io_return

port_read_dword:
    in   eax, dx
    jmp  short port_io_return

port_write_trampoline:
    mov  eax, [ebp + 0x10]
    bt   ebx, 2
    jc   short port_write_dword
    bt   ebx, 1
    jc   short port_write_word
    jmp  short port_write_byte

port_read_trampoline:
    bt   ebx, 2
    jc   short port_read_dword
    bt   ebx, 1
    jc   short port_read_word
    jmp  short port_read_byte

port_io_write:
port_io_read:
    push ebp
    mov  ebp, esp
    push ebx
    mov  ebx, [ebp + 0x8]
    mov  edx, [ebp + 0xC]
    bt   ebx, 3
    jc   short port_write_trampoline
    jnc  short port_read_trampoline
port_io_return:
    pop  ebx
    pop  ebp
    ret

nmi_enable:
    push ebp
    mov  ebp, esp
    mov  al, [shadow_p70]
    and  al, 0x7F
    out  0x70, al
    mov  [shadow_p70], al
    pop  ebp
    ret

nmi_disable:
    push ebp
    mov  ebp, esp
    mov  al, [shadow_p70]
    or   al, 0x80
    out  0x70, al
    mov  [shadow_p70], al
    pop  ebp
    ret

gdt_install:
    push ebp
    mov  ebp, esp
    mov  eax, [ebp + 0x8]
    mov  eax, [eax]
    lgdt [eax]
    pop  ebp
    ret

idt_install:
    push ebp
    mov  ebp, esp
    mov  eax, [ebp + 0x8]
    mov  eax, [eax]
    lidt [eax]
    pop  ebp
    ret

get_stack_base:
    mov  eax, ebp
    ret

; halt and catch fire
hcf:
    cli
halt:
    hlt
    jmp short halt

ints_flag_clear:
    cli
    ret

ints_flag_set:
    sti
    ret
