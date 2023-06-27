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

%ifndef MMAP_S
%define MMAP_S

%include "bios.s"

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
    mov   edi, 0x00000000

    xor   eax, eax
    push  eax

    xor   ebx, ebx
loop:
    ; clear ACPI 3.0 attribute field if BIOS doesn't fill in
    mov   dword [es:edi+0x14], 0x00 
    mov   eax, 0x0000E820
    mov   ecx, 0x00000018
    mov   edx, 0x534D4150

    int   0x15
    jnc   mmapcnt1
    push  mmap_err1
    call  error

mmapcnt1:
    cmp   eax, 0x534D4150
    je    mmapcnt2
    push  mmap_err2
    call  error

mmapcnt2:
    cmp   ebx, 0x00
    je    mmap_done

    cmp   ecx, 0x14
    je    mmapcnt3
    cmp   ecx, 0x18
    je    mmapcnt3

    push  mmap_err2
    call  error

mmapcnt3:
    add   di, 0x18
    pop   eax
    inc   eax
    push  eax
    jmp   loop

mmap_done:
    mov   [mmap_seg], es
    mov   [mmap_off], di

    pop   dword eax
    pop   word es
    pop   dword edi
    pop   dword edx
    pop   dword ecx
    pop   dword ebx
    pop   dword eax
    pop   dword ebp

    ret

mmap_err1 db "Error: BIOS does not support int 0x15 0xE820",0x0D,0x0A,0
mmap_err2 db "Error: mmap failed, BIOS gave malformed request",0x0D,0x0A,0

%endif
