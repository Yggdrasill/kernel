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

section .boot
bits   16
extern __start

jmp   short $+__entry - $
times 3-($-$$) db 0x90
name                  db    "fake.bpb"
bytes_per_sector      dw    512
sectors_per_cluster   db    1
reserved_sectors      dw    1
fat                   db    2
root_dirs             dw    224
sectors               dw    2880
media_type            db    0xf8
fat_sectors           dw    9
sectors_per_track     dw    18
heads                 dw    2
hidden_sectors        dd    0
huge_sectors          dd    0
current_drive         db    0
reserved              db    0
signature             db    0x29
volume                dd    0x2d7e5a1a
label                 db    "FAKEBPB    "
filesystem            db    "FAT12   "

__entry:
cli
mov   ax, 0x7000
mov   ss, ax
mov   bp, 0xFFFF
mov   sp, bp

xor   ax, ax
mov   ds, ax
mov   es, ax
mov   si, 0x7C00
mov   di, 0x7E00

mov   cx, 0x0200
rep   movsb

xor   si, si
xor   di, di

sti

jmp   boot

%include "bios.s"

boot:
    call  vga_page_rst
    call  cursor_rst
    mov   byte [drive], dl
    xor   word dx, dx
    mov   byte dl, [drive]
    push  word dx
    call  reset
    mov   word sp, 0xFFFF    ; flush stack

    xor   word dx, dx
    mov   byte dl, [drive]
    push  word 0x8000
    push  word 0x0240 ; read 32K from disk
    push  word dx
    call  read
    mov   sp, 0xFFFF

    jmp   0x0000:stage15

drive     db 0

times     446 - ($ - $$) db 0
part0     times 16 db 0
part1     times 16 db 0
part2     times 16 db 0
part3     times 16 db 0

dw        0xAA55

section .stage15

%include "a20.s"
%include "mmap.s"
%include "pmode.s"
%include "vga.s"
%include "pic.s"

stage15:
    call  init_video

    push  s15_str
    push  s15s_len
    call  print
    add   sp, 4

    call  a20_init
    call  mmap

    ; Clear EFLAGS register

    push  dword 0x02
    popfd

    call  mask_ints

    cli

    xor ax, ax

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

    ; push memory map

    push word [mmap_seg]
    push word [mmap_off]

    jmp 0x0008:__start

s15_str   db "Entering stage 1.5",0x0D,0x0A
s15s_len  equ $ - s15_str

mmap_seg  dw 0
mmap_off  dw 0
