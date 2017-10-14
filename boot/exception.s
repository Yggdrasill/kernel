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

extern exception_handler

global exception_unknown
global exception_0x00
global exception_0x01
global exception_0x02
global exception_0x03
global exception_0x04
global exception_0x05
global exception_0x06
global exception_0x07
global exception_0x08
global exception_0x0A
global exception_0x0B
global exception_0x0C
global exception_0x0D

exception_wrapper:
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

    call  exception_handler

    add   esp, 0x04
    pop   fs
    pop   gs
    pop   es
    pop   ds
    popa

    add   esp, 0x08

    iret

exception_unknown:
    push  byte 0x00
    push  byte 0x1F

    jmp   exception_wrapper

exception_0x00:
    push  byte 0x00
    push  byte 0x00

    jmp   exception_wrapper

exception_0x01:
    push  byte 0x00
    push  byte 0x01

    jmp   exception_wrapper

exception_0x02:
    push  byte 0x00
    push  byte 0x02

    jmp   exception_wrapper

exception_0x03:
    push  byte 0x00
    push  byte 0x03

    jmp   exception_wrapper

exception_0x04:
    push  byte 0x00
    push  byte 0x04

    jmp   exception_wrapper

exception_0x05:
    push  byte 0x00
    push  byte 0x05

    jmp   exception_wrapper

exception_0x06:
    push  byte 0x00
    push  byte 0x06

    jmp   exception_wrapper

exception_0x07:
    push  byte 0x00
    push  byte 0x07

    jmp   exception_wrapper

exception_0x08:
    push  byte 0x08

    jmp   exception_wrapper

exception_0x0A:
    push  byte 0x0A

    jmp   exception_wrapper

exception_0x0B:
    push  byte 0x0B

    jmp   exception_wrapper

exception_0x0C:
    push  byte 0x0C

    jmp   exception_wrapper

exception_0x0D:
    push  byte 0x0D

    jmp   exception_wrapper
