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

__bios_print:
    push  bp
    mov   bp, sp
    push  ax
    push  bx
    push  cx
    push  dx
    mov   ax, 0x0300
    xor   bx, bx
    int   0x10
    mov   cx, [ss:bp + 4]
    mov   ax, [ss:bp + 6]
    mov   bx, 0x0007
    push  bp
    mov   bp, ax
    mov   ax, 0x1301
    int   0x10
    pop   bp
    pop   dx
    pop   cx
    pop   bx
    pop   ax
    pop   bp 
    ret

__bios_error:
    push  bp
    mov   bp, sp
    mov   si, [ss:bp + 6]  ; push error message again
    push  si
    mov   si, [ss:bp + 4] 
    push  si
    call  __bios_print
    cli
    hlt
