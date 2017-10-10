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

%ifndef PMODE_S
%define PMODE_S

gdt_install:
    lgdt  [es:gdt_info]
    ret

idt_install:
    lidt  [idt_info]
    ret

pmode_init:
    push  eax

    mov   eax, cr0
    or    eax, 1
    mov   cr0, eax

    pop   eax
    ret

gdt_info:
gdt_size  dw  0x18 - 1
gdt_ptr   dd  0x7E00 + gdt

gdt:
null_gdt  times 8 db 0
code_gdt  db 0xFF,0xFF,0x00,0x00,0x00,0x9A,0xCF,0x00
data_gdt  db 0xFF,0xFF,0x00,0x00,0x00,0x92,0xCF,0x00

idt_info:
idt_size  dw 0
idt_ptr   dd 0

%endif
