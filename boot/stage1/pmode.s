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

global gdt_install
global idt_install
global pmode_init

bits    16
section .stage15 alloc exec progbits

gdt_install:
    push  bp
    mov   bp, sp
    push  ax
    push  di
    mov   ax, es
    shl   ax, 4
    add   ax, [gdt_ptr]
    mov   [gdt_ptr], ax
    mov   di, gdt_info
    lgdt  [es:di]
    pop   ax
    pop   di
    pop   bp
    ret

idt_install:
    push  bp
    mov   bp, sp
    push  di
    mov   di, idt_info
    lidt  [es:di]
    pop   di
    pop   bp
    ret

pmode_init:
    push  bp
    mov   bp, sp
    push  eax

    mov   eax, cr0
    or    eax, 1
    mov   cr0, eax

    pop   eax
    pop   bp
    ret

gdt_info:
gdt_size  dw  0x18 - 1
gdt_ptr   dd  gdt

gdt:
null_gdt  times 8 db 0
code_gdt  db 0xFF,0xFF,0x00,0x00,0x00,0x9A,0xCF,0x00
data_gdt  db 0xFF,0xFF,0x00,0x00,0x00,0x92,0xCF,0x00

idt_info:
idt_size  dw 0
idt_ptr   dd 0
