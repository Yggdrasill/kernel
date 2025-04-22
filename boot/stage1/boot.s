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

global mmap_seg
global mmap_off

extern vga_page_rst
extern cursor_rst
extern init_video
extern reset
extern read
extern a20_init
extern mmap
extern error
extern mask_ints
extern idt_install
extern gdt_install
extern pmode_init

bits   16

section .boot alloc exec progbits nowrite

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
mov   bp, 0xFFF0
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

boot:
    call  vga_page_rst
    call  cursor_rst
    call  init_video

    mov   byte [drive], dl
    xor   word dx, dx
    mov   byte dl, [drive]
    push  word dx
    call  reset
    mov   word sp, 0xFFF0    ; flush stack

    xor   word dx, dx
    mov   byte dl, [drive]
    push  word 0x8000
    push  word 0x0240 ; read 32K from disk
    push  word dx
    call  read
    mov   sp, 0xFFF0

    jmp   0x0000:stage15

drive     db 0

section .mbr alloc noexec progbits write
part0     times 16 db 0
part1     times 16 db 0
part2     times 16 db 0
part3     times 16 db 0

dw        0xAA55

section .stage15 alloc exec progbits nowrite

stage15:
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

    jmp   0x0008:i386
i386:
bits 32
    call  read_elf
    mov   dword edx, dword [elf_entry]

    ; Clean up all registers

    xor   eax, eax
    xor   ebx, ebx
    xor   ecx, ecx

    ; push memory map

    push  word [mmap_seg]
    push  word [mmap_off]

    push  edx
    ret

%define         PT_LOAD_TYPE      0x01

%define         PH_TYPE_OFFSET    0x00
%define         PH_FILE_OFFSET    0x04
%define         PH_VIRT_ADDR      0x08
%define         PH_FILE_SIZE      0x10

%define         SH_NOBITS_TYPE    0x08
%define         SH_PROGBITS_TYPE  0x01

%define         SH_TYPE_OFFSET    0x04
%define         SH_FILE_OFFSET    0x10
%define         SH_FILE_SIZE      0x14


read_elf:
    push        ebp
    mov         ebp, esp
    push        eax
    push        ebx
    push        ecx
    push        edx
    push        esi
    push        edi

    ; Read ELF and find e_shstrndx section.

    xor         eax, eax
    mov         ax, [e_shentsize]
    mul   word  [e_shstrndx]
    add         ax, [e_shoff]
    mov         ebx, ei_mag + SH_FILE_OFFSET
    add         ebx, eax
    mov         eax, [ebx]
    add         eax, ei_mag

    ; Now find the ._init section by text match

    mov         bx, [e_shoff]
    add         ebx, ei_mag
    xor         edx, edx
init_loop:
    add         bx, [e_shentsize] ; skip SHN_UNDEF
    inc         edx
    cmp   dword [ebx + SH_TYPE_OFFSET], SH_PROGBITS_TYPE
    je          test_init
    cmp         edx,  [e_shnum]
    jl          init_loop
    cli
    hlt ; TODO: hang for now, will implement later
test_init:
    mov         esi,  [ebx]
    add         esi,  eax
    mov         edi,  init_str
    mov         ecx,  init_slen
    rep         cmpsb
    jne         init_loop

    ; Before relocating PT_LOAD segments, we
    ; must ensure that the ELF headers are all
    ; relocated to a preserved region.

    mov         esi,  ei_mag
    mov         edi,  _elf_header 
    mov         cx,   [e_ehsize]
    rep         movsb

    mov         esi,  ei_mag
    mov         edi,  _elf_header
    add         esi,  [e_phoff]
    add         edi,  [e_phoff]
    movzx word  ax,   [e_phentsize]
    mul   word  [e_phnum] 
    mov         ecx,  eax
    rep         movsb

    mov         esi,  ei_mag
    mov         edi,  _elf_header
    add         esi,  [e_shoff]
    add         edi,  [e_shoff]
    movzx word  ax,   [e_shentsize]
    mul   word  [e_shnum] 
    mov         ecx,  eax
    rep         movsb

    ; Now relocate all segments to preserved memory

    mov         eax,  ei_mag
    mov         ebx,  _elf_header
    add         eax,  [e_phoff]
    add         ebx,  [e_phoff]
    mov         dx,   [e_phnum]
ph_reloc:
    mov         cx,   [eax + PH_FILE_SIZE]
    mov         esi,  [eax + PH_FILE_OFFSET]
    add         esi,  ei_mag
    mov         edi,  [ebx + PH_FILE_OFFSET]
    add         edi,  _elf_header
    rep         movsb
    dec         dx
    cmp         dx,   0
    jne         ph_reloc

    mov         eax,  ei_mag
    mov         ebx,  _elf_header
    add         eax,  [e_shoff]
    add         ebx,  [e_shoff]
    mov         dx,   [e_shnum]
sh_reloc:
    cmp   word  [eax + SH_TYPE_OFFSET], SH_NOBITS_TYPE ; Skip SHT_NOBITS
    je          shr_cont
    mov         cx,   [eax + SH_FILE_SIZE]
    mov         esi,  [eax + SH_FILE_OFFSET]
    mov         edi,  [ebx + SH_FILE_OFFSET]
    rep         movsb
shr_cont:
    dec         dx
    cmp         dx,   0
    jne         sh_reloc

    ; Now read the program headers and relocate
    ; to the address specified by p_vaddr.

    xor         ebx,  ebx
    mov         eax,  [elf_phoff]
    mov         bx,   [elf_phnum]
ph_loop:
    cmp         ebx,  0
    je          phlp_exit
    mov         edi,  _elf_header
    add         edi,  eax
    cmp         dword [edi], dword PT_LOAD_TYPE ; Only PT_LOAD
    jne         phlp_next
    mov   dword ecx,  [edi + PH_FILE_SIZE]
    mov         esi,  _elf_header
    add         esi,  [edi + PH_FILE_OFFSET]
    mov         edi,  [edi + PH_VIRT_ADDR]
    rep         movsb
phlp_next:
    add         ax, [elf_phentsize]
    dec         ebx
    jmp         ph_loop
phlp_exit:
    pop         edi
    pop         esi
    pop         edx
    pop         ecx
    pop         ebx
    pop         eax
    pop         ebp
    ret

section     .rodata
init_str    db "._init"
init_slen   equ $ - init_str

elf_err     db "E: ELF not found!",0x0D,0x0A
elf_len     equ $ - elf_err
;sht_err     db "No SHT_PROGBITS section header!",0x0D,0x0A
;sht_len     equ $ - sht_err
init_err    db "E: ._init missing!",0x0D,0x0A
init_elen   equ $ - init_err

section     .data
mmap_seg    dw 0
mmap_off    dw 0

_elf_init     dd 0
_elf_shstrndx dd 0

section .stage2.bss alloc noexec nobits write
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

section .elf alloc noexec nobits write
_elf_header:
elf_mag:        resd 1
elf_ident:      resb 12
elf_type:       resw 1
elf_machine:    resw 1
elf_version:    resd 1
elf_entry:      resd 1
elf_phoff:      resd 1
elf_shoff:      resd 1
elf_flags:      resd 1
elf_ehsize:     resw 1
elf_phentsize:  resw 1
elf_phnum:      resw 1
elf_shentsize:  resw 1
elf_shnum:      resw 1
elf_shstrndx:   resw 1

