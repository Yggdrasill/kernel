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

global mmap
extern bios_error

extern mmap_seg
extern mmap_off

bits    16
section .stage15 alloc exec progbits nowrite

mmap:
    push  dword ebp
    mov   ebp, esp 
    push  dword eax
    push  dword ebx
    push  dword ecx
    push  dword edx
    push  dword edi
    push  word es

    mov   eax, 0x000027E0
    mov   es, ax
    xor   edi, edi
    xor   ebx, ebx
loop:
    ; clear ACPI 3.0 attribute field if BIOS doesn't fill in
    mov   dword [es:edi+0x14], 0x00 
    mov   eax, 0x0000E820
    mov   ecx, 0x00000018
    mov   edx, 0x534D4150

    int   0x15
    jc    mmap_e1
    cmp   eax, 0x534D4150
    jne   mmap_e2
    cmp   ebx, 0x00
    je    mmap_done

    cmp   ecx, 0x14
    je    mmap_continue
    cmp   ecx, 0x18
    jne   mmap_e2
mmap_continue:
    add   di, 0x18
    jmp   loop
mmap_done:
    mov   [mmap_seg], es
    mov   [mmap_off], di

    pop   word es
    pop   dword edi
    pop   dword edx
    pop   dword ecx
    pop   dword ebx
    pop   dword eax
    pop   dword ebp

    ret

mmap_e1:
    push  mmap_err1
    push  me1_len
    call  bios_error

mmap_e2:
    push  mmap_err2
    push  me2_len
    call  bios_error

section .rodata
mmap_err1 db "E: E820 not supported.",0x0D,0x0A
me1_len   equ $ - mmap_err1
mmap_err2 db "E: E820 malformed response",0x0D,0x0A
me2_len   equ $ - mmap_err2
