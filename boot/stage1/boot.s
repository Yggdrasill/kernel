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

section .boot
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
%include "vga.s"

boot:
    call  vga_page_rst
    call  cursor_rst
    call  init_video

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

    call  a20_init
    call  mmap

    ; Verify that the ELF bootloader is present
    ; by testing against magic header of the
    ; ELF format.

    mov   eax, [ei_mag]
    cmp   eax, 0x464C457F
    je    s15_continue
    push  elf_err
    push  elf_len
    call  error

s15_continue:
    ; Read ELF file and find e_shstrndx section.

    xor   eax, eax
    mov   ax, [es:e_shentsize]
    mul   word [es:e_shstrndx]
    add   eax, [es:e_shoff]
    mov   ebx, ei_mag + 0x10
    add   ebx, eax
    mov   eax, [es:ebx]
    add   eax, ei_mag

    ; Now find the ._init section by text match

    mov   ebx, [es:e_shoff]
    add   ebx, ei_mag
    xor   edx, edx
init_loop:
    add   bx, [es:e_shentsize] ; skip SHN_UNDEF
    inc   dx
    cmp   dword [es:ebx + 4], 1
    je    test_init
    cmp   dx, [es:e_shnum]
    jl    init_loop
    cli
    hlt ; TODO: hang for now, will implement later
test_init:
    mov   esi, [ebx]
    add   si, ax
    mov   di, init_str
    mov   ecx, init_slen
    rep   cmpsb
    jne   init_loop

    ; Found ._init section, save offset for now
    mov   ax, [e_shentsize]
    mul   dx
    mov   bx, [e_shoff]
    add   ebx, ei_mag + 0x10
    add   ebx, eax
    mov   eax, [ebx]
    mov   [_elf_init], eax

    ; We must first calculate size of the file.
    ; Because the linker has been configured to
    ; not align, this is easier than it woulld
    ; otherwise be.

    xor   si, si
    mov   si, [e_phoff]
    add   si, ei_mag + 0x10
    xor   ecx, ecx
    mov   bx, [e_phnum]
phsize_loop:
    add   cx, [es:si]
    dec   bx
    add   si, [e_phentsize]
    cmp   bx, 0
    jne   phsize_loop

    mov   si, [e_shoff]
    add   si, ei_mag + 0x14
    mov   bx, [e_shnum]
shsize_loop:
    add   cx, [es:si]
    dec   bx
    add   si, [e_shentsize]
    cmp   bx, 0
    jne   shsize_loop

    mov   ax, [e_phentsize]
    mul   word [e_phnum]
    add   cx, ax
    mov   ax, [e_shentsize]
    mul   word [e_shnum]
    add   cx, ax

    ; Proceed to relocate

    mov   edx, [e_entry]
    mov   si, ei_mag
    add   si, [_elf_init]
    mov   di, dx
    rep movsb

    ; Clear EFLAGS register

    push  dword 0x02
    popfd

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

    ; push memory map

    push  word [mmap_seg]
    push  word [mmap_off]

    push  dword 0x0008
    push  word dx
    retf

init_str    db "._init"
init_slen   equ $ - init_str

elf_err     db "E: ELF not found!",0x0D,0x0A
elf_len     equ $ - elf_err
;sht_err     db "No SHT_PROGBITS section header!",0x0D,0x0A
;sht_len     equ $ - sht_err
init_err    db "E: ._init missing!",0x0D,0x0A
init_elen   equ $ - init_err

mmap_seg    dw 0
mmap_off    dw 0

_elf_init     dd 0
_elf_shstrndx dd 0

times     1024 - ($ - $$) db 0

section .bss
ei_mag:       resd 1
e_ident:      resb 12
e_type:       resw 1
e_machine:    resw 1
e_version:    resd 1
e_entry:      resd 1
e_phoff:      resd 1
e_shoff:      resd 1
e_flags:      resd 1
e_ehsize:     resw 1
e_phentsize:  resw 1
e_phnum:      resw 1
e_shentsize:  resw 1
e_shnum:      resw 1
e_shstrndx:   resw 1
