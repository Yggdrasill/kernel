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
extern pmode_init
extern pmode_exit
extern rmode_trampoline

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
    push  dword 0x02
    popfd

    call  a20_init
    call  pmode_init
bits 32
    ; Start a fresh stack frame for 32-bit
    ; protected mode. Stack is aligned on
    ; 16-byte boundary to make various 
    ; compilers happy. 

    mov   esp, 0x7FFF0
    mov   ebp, 0x7FFF0
    
    push  mmap
    call  rmode_trampoline
    add   esp, 4

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

    ; Verify that the ELF bootloader is present
    ; by testing against magic header of the
    ; ELF format.

    mov   eax, [ei_mag]
    cmp   eax, 0x464C457F
    je    header_ok
    push  word elf_err
    push  word elf_len
    push  error
    call  rmode_trampoline

header_ok:
    sub         esp,  0x10
    mov         esi,  [e_phoff]
    mov         edi,  [e_shoff]
    lea         eax,  [esi + ei_mag]
    lea         ebx,  [esi + _elf_header]
    lea         ecx,  [edi + ei_mag]
    lea         edx,  [edi + _elf_header]

    mov         [ebp - 0x1C], eax
    mov         [ebp - 0x20], ebx
    mov         [ebp - 0x24], ecx
    mov         [ebp - 0x28], edx

    ; Before relocating PT_LOAD segments, we
    ; must ensure that the ELF headers are all
    ; relocated to a preserved region.

    mov         esi,  ei_mag
    mov         edi,  _elf_header 
    movzx       ecx,  word [e_ehsize]
    rep         movsb

    mov         esi,  [ebp - 0x1C]
    mov         edi,  [ebp - 0x20]
    movzx       ecx,  word [e_phentsize]
    imul        cx,   word [e_phnum] 
    rep         movsb

    mov         esi,  [ebp - 0x24]
    mov         edi,  [ebp - 0x28]
    movzx       ecx,  word [e_shentsize]
    imul        cx,   word [e_shnum] 
    rep         movsb

    ; Find e_shstrndx section

    movzx       ebx,  word [e_shentsize]
    imul        bx,   word [e_shstrndx]
    add         ebx,  [ebp - 0x24]
    add         ebx,  SH_FILE_OFFSET
    mov         ebx,  [ebx]
    add         ebx,  ei_mag

    mov         eax,  [ebp - 0x24]
    movzx       edx,  word [e_shnum]
sh_reloc:
    cmp   word  [eax + SH_TYPE_OFFSET], SH_NOBITS_TYPE ; Skip SHT_NOBITS
    je          shr_cont
    mov         ecx,  [eax + SH_FILE_SIZE]
    mov         esi,  [eax + SH_FILE_OFFSET]
    add         esi,  ei_mag
    mov         edi,  [eax + SH_FILE_OFFSET]
    add         edi,  _elf_header
    rep         movsb
    
    ; Skip if ._init section found.

    cmp         [init_found], byte 0x1
    je          shr_cont
    
    ; String match section name with ._init.

    lea         esi,  [ebx]
    add         esi,  [eax]
    mov         edi,  init_str
    mov         ecx,  init_slen
    rep         cmpsb
    jne         shr_cont
    mov         [init_found], byte 0x1
shr_cont:
    add         ax,   [e_shentsize]
    dec         edx
    test        edx,  edx
    jnz         sh_reloc

    ; Now read the program headers and relocate
    ; to the address specified by p_vaddr. Since
    ; this most definitely clobbers the ELF structure
    ; we have to do this after moving the ELF to a
    ; safe location.

    mov         eax,  [elf_phoff]
    movzx       ebx,  word [elf_phnum]

    ; Check if ._init was found, and error if it wasn't.

    cmp         [init_found], byte 0x1
    je          ph_loop
    push        word init_err
    push        word init_elen
    push        error
    call        rmode_trampoline

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
    add         esp, 0x10
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

init_found  db 0

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

