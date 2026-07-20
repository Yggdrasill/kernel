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

global __bios_print
global __bios_error
bits    16
section .boot.util alloc exec progbits nowrite

bios_print:
__bios_print:
    push  bp
    mov   bp, sp
    push  ax
    push  bx
    push  cx
    push  dx
    push  es
    mov   ax, 0x0300
    xor   bx, bx
    mov   es, bx
    int   0x10
    mov   ecx, dword [ss:bp + 8]
    mov   eax, dword [ss:bp + 4]
    mov   bx, 0x0007
    push  bp
    mov   bp, ax
    mov   ax, 0x1301
    int   0x10
    pop   bp
    pop   es
    pop   dx
    pop   cx
    pop   bx
    pop   ax
    pop   bp 
    ret

bios_error:
__bios_error:
    add      sp, 2
    call  bios_print
    cli
    hlt
