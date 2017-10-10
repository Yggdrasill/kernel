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

bits   16

cli
mov   ax, 0x7000
mov   ss, ax
mov   sp, 0xFFFF

mov   ax, 0x07E0
mov   es, ax
mov   ax, 0x07C0
mov   ds, ax
xor   si, si
xor   di, di

mov   cx, 0x0200
rep   movsb

mov   ax, 0x07E0
mov   ds, ax
mov   es, ax
xor   di, di
xor   si, si

sti

jmp   0x07E0:boot

%include "bios.s"

boot:
    mov   [drive], dl

    xor   dx, dx
    mov   dl, [drive]
    push  dx
    call  reset
    mov   sp, 0xFFFF    ; flush stack

    xor   dx, dx
    mov   dl, [drive]
    push  word 0x07E0
    push  word 0x0200
    push  word 0x023C   ; read a ridiculous amount of sectors
    push  dx
    call  read
    mov   sp, 0xFFFF

    jmp   stage15

drive     db 0

times     446 - ($ - $$) db 0

part0     times 16 db 0
part1     times 16 db 0
part2     times 16 db 0
part3     times 16 db 0

dw        0xAA55

%include "a20.s"
%include "mmap.s"
%include "pmode.s"
%include "vga.s"
%include "pic.s"

stage15:
    call  init_video

    push  s15_str
    call  print
    add   sp, 2

    call  a20_init
    call  mmap

    call  mask_ints

    cli

    call  idt_install
    call  gdt_install
    call  pmode_init

    ; Initialize segment registers to use the GDT.
    ; There should be no more 16 bit or real mode
    ; code executed after this point, except for
    ; these simple mov instructions.

    mov   ax, 0x0010
    mov   ss, ax
    mov   es, ax
    mov   ds, ax
    mov   gs, ax
    mov   fs, ax

    ; Stack is aligned on 16-byte boundary
    ; to make various compilers happy
    ; This also initializes the registers
    ; for 32 bit execution.

    mov   esp, 0x7FFF0
    mov   ebp, 0x7FFF0
    mov   esi, 0x17E00
    mov   edi, 0x17E00

    ; Clean up all registers

    xor   eax, eax
    xor   ebx, ebx
    xor   ecx, ecx
    xor   edx, edx

    jmp   0x0008:0x8400

    ; If we ever come to this point,
    ; just give up

    hlt

s15_str   db "Entering stage 1.5",0x0D,0x0A,0

mmap_seg  dw 0
mmap_off  dw 0
