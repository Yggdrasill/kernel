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

extern irq_handler

global irq_0x00
global irq_0x01
global irq_0x02
global irq_0x03
global irq_0x04
global irq_0x05
global irq_0x06
global irq_0x07
global irq_0x08
global irq_0x09
global irq_0x0A
global irq_0x0B
global irq_0x0C
global irq_0x0D
global irq_0x0E
global irq_0x0F

global irq_wrapper

irq_0x00:
    push  0x00
    push  0x00

    jmp   irq_wrapper

irq_0x01:
    push  0x00
    push  0x01

    jmp   irq_wrapper

irq_0x02:
    push  0x00
    push  0x02

    jmp   irq_wrapper

irq_0x03:
    push  0x00
    push  0x03

    jmp   irq_wrapper

irq_0x04:
    push  0x00
    push  0x04

    jmp   irq_wrapper

irq_0x05:
    push  0x00
    push  0x05

    jmp   irq_wrapper


irq_0x06:
    push  0x00
    push  0x06

    jmp   irq_wrapper


irq_0x07:
    push  0x00
    push  0x07

    jmp   irq_wrapper

irq_0x08:
    push  0x00
    push  0x08

    jmp   irq_wrapper

irq_0x09:
    push  0x00
    push  0x09

    jmp   irq_wrapper

irq_0x0A:
    push  0x00
    push  0x0A

    jmp   irq_wrapper

irq_0x0B:
    push  0x00
    push  0x0B

    jmp   irq_wrapper

irq_0x0C:
    push  0x00
    push  0x0C

    jmp   irq_wrapper

irq_0x0D:
    push  0x00
    push  0x0D

    jmp   irq_wrapper

irq_0x0E:
    push  0x00
    push  0x0E

    jmp   irq_wrapper

irq_0x0F:
    push  0x00
    push  0x0F

    jmp   irq_wrapper

irq_wrapper:
    cli

    pusha
    push  ds
    push  es
    push  gs
    push  fs

    mov   ax, 0x10
    mov   ds, ax
    mov   es, ax
    mov   gs, ax
    mov   fs, ax

    push  esp
    call  irq_handler
    add   esp, 4

    pop   fs
    pop   gs
    pop   es
    pop   ds
    popa
    add   esp, 8

    sti

    iret
