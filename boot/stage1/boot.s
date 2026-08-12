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

extern __BOOT_ENTRY
extern __BOOT_ADDR
extern __BOOT_SIZE
extern __STAGE15_LOAD_SEG
extern __STAGE15_LOAD_OFF

extern disk_geometry
extern reset
extern read
extern a20_init
extern mmap
extern store_bios_imr
extern mask_ints
extern pmode_init
extern rmode_trampoline_no_sret

extern drive
extern shadow_p70

extern __bios_error
extern __chs_geometry
; Currently not used, but ideally...
extern __disk_reset
extern __chs_read

bits   16

section .boot alloc exec progbits nowrite

jmp short $+__entry - $
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
    mov   sp, 0xFFF0
    mov   bp, sp
    sti
    pusha

    ; VGA init
    mov   ax, 0x03
    int   0x10
    mov   ah, 0x01
    mov   cx, 0x3F00
    int   0x10

    ; VGA page reset
    mov   ax, 0x0500
    int   0x10

    ; VGA cursor reset
    mov   ah, 0x02
    xor   bx, bx
    xor   dx, dx
    int   0x10

    popa
    cld

    push  dword 0x00
    pop   es
    pop   ds
    mov   si, __BOOT_ENTRY
    mov   di, __BOOT_ADDR

    mov   cx, __BOOT_SIZE
    rep   movsb

    push  dx
    push  dx
    ; push dx for stage 1.5
    ; disk reads
    push  dx
    call  disk_geometry
    pop   dx
    call  reset

    push  word __STAGE15_LOAD_SEG
    pop   es
    mov   bx, __STAGE15_LOAD_OFF
    mov   cx, 0x02
    pop   dx
    mov   si, 0x03
    call  read

    jmp   0x0000:stage15

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

    ; Enable A20 line, store the BIOS
    ; interrupt mask, and then mask all
    ; ints for transition to protected
    ; mode.
    cli
    call  a20_init
    call  store_bios_imr
    call  mask_ints

    ; Disable NMI and store shadow state
    mov   al, 0x80
    out   0x70, al
    mov   [shadow_p70], al

    call  0x0000:pmode_init
bits 32
    ; Get boot drive number
    pop   word dx
    movzx edx, dx

    ; Start a fresh stack frame for 32-bit
    ; protected mode. Stack is aligned on
    ; 16-byte boundary to make various 
    ; compilers happy. 
    mov   esp, 0x7FFF0
    mov   ebp, esp

    call  read_elf

    xor   eax, eax
    xor   ebx, ebx
    xor   ecx, ecx

    ; push __start
    push  [e_entry]
    ret

%define         PT_LOAD_TYPE      0x01

%define         PH_TYPE_OFFSET    0x00
%define         PH_FILE_OFFSET    0x04
%define         PH_VIRT_ADDR      0x08
%define         PH_FILE_SIZE      0x10
%define         PH_MEM_SIZE       0x14

%define         SH_NOBITS_TYPE    0x08
%define         SH_PROGBITS_TYPE  0x01

%define         SH_TYPE_OFFSET    0x04
%define         SH_FILE_OFFSET    0x10
%define         SH_FILE_SIZE      0x14

%include "s1_generated.s"

read_wrapper:
    ; easy space saving, even if all
    ; that needs saving is eax/ecx/edx
    pusha

    push  [ebp - ABI_DISK_SIZEOF - 4]
    push  esi
    push  edi
    push  ebx
    lea   eax, [ebp - ABI_DISK_SIZEOF]
    push  eax
    push  __chs_read
    call  rmode_trampoline_no_sret
    add   esp, 0x18
    shr   eax, 20
    jz    wrapper_ret
    ; ECC corrected
    cmp   eax, 0x11
    je    wrapper_ret
elf_read_error:
    push  dword elf_len
    push  dword elf_err
    push  __bios_error
    call  rmode_trampoline_no_sret
wrapper_ret:
    popa
    ret

read_elf:
    ; enter consumes 4 bytes, whereas
    ; the following consumes 6 bytes:
    ; push r32
    ; mov  r/m32, r32
    ; sub  r/m32, imm8
    enter ABI_DISK_SIZEOF + 4, 0
    ; save edx drive number
    mov   [ebp - ABI_DISK_SIZEOF - 4], edx
    push  edx
    lea   eax, [ebp - ABI_DISK_SIZEOF]
    push  eax
    push  __chs_geometry
    call  rmode_trampoline_no_sret
    add   esp, 0x0C

    or    eax, eax
    jnz   elf_read_error

    mov   esi, ABI_STAGE2_LBA
    ; read 1 sector
    mov   edi, 1
    mov   ebx, ei_mag
    call  read_wrapper

    mov   eax, [ei_mag]
    cmp   eax, 0x464C457F
    jne   elf_read_error

    ; Probably unnecessary, but if there
    ; are a ridiculous number of program
    ; headers, we must read them all.
    ; The alternative is to assume that
    ; all program headers reside in the
    ; ELF header LBA.
    movzx eax, word [e_phentsize]
    movzx ecx, word [e_phnum]
    or    ecx, ecx
    jz    elf_read_error
    imul  ecx
    add   eax, [e_phoff]
    lea   edx, [eax + ei_mag]
    xor   eax, eax
    mov   ah, 2
    mov   esi, 5
    mov   edi, 1
elf_ph_read:
    ; Now read all headers one sector
    ; at a time... because this saves
    ; space even if it's slow.
    lea   ebx, [eax + ei_mag]
    cmp   edx, ebx
    jbe   elf_ph_done
    call  read_wrapper
    inc   esi
    add   eax, 0x200
    jmp   elf_ph_read
elf_ph_done:
    mov   edi, [e_phoff]
    add   edi, ei_mag
elf_pseglp:
    push  ecx
    cmp   [edi + PH_TYPE_OFFSET], PT_LOAD_TYPE
    jne   elf_pseg_next

    ; Calculate file offset from ELF LBA.
    ; This is setup to read the entire
    ; program header into the buffer
    ; which is pointed to by ebx.
    mov   eax, 0x200 * ABI_STAGE2_LBA
    add   eax, [edi + PH_FILE_OFFSET]
    mov   ecx, 0x200
    xor   edx, edx
    div   ecx
    ; Store LBA + byte offset.
    mov   esi, eax
    push  edx
    mov   eax, 0x200 * ABI_STAGE2_LBA
    add   eax, [edi + PH_FILE_OFFSET]
    add   eax, [edi + PH_FILE_SIZE]
    xor   edx, edx
    div   ecx
    or    edx, edx
    jz    elf_ptload_read
    inc   eax
elf_ptload_read:
    ; edx = byte offset
    ; esi = LBA
    ; edi = blocks to read
    pop   edx
    sub   eax, esi
    push  edi
    mov   edi, eax
    call  read_wrapper
    pop   edi

    ; Finally it's time to actually
    ; relocate the program segment
    ; from the buffer to its p_vaddr.
    mov   esi, ebx
    add   esi, edx
    push  edi
    mov   ecx, [edi + PH_FILE_SIZE]
    mov   eax, [edi + PH_MEM_SIZE]
    push  ecx
    mov   edi, [edi + PH_VIRT_ADDR]
    rep   movsb
    ; Zero BSS
    pop   ecx
    sub   eax, ecx
    js    elf_read_error
    mov   ecx, eax
    xor   eax, eax
    rep   stosb
elf_pseg_next:
    pop   edi
    pop   ecx
    movzx eax, word [e_phentsize]
    add   edi, eax
    loop  elf_pseglp
    leave
    ret

section .stage15.rodata
elf_err db "E: Stage 2 not found!",0x0D,0x0A
elf_len equ $ - elf_err

section .elf alloc noexec nobits write
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
