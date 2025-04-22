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

global  vga_page_rst
global  cursor_rst
global  print
global  error
global  reset
global  read

bits    16
section .boot.util alloc exec progbits nowrite

vga_page_rst:
    push  bp
    mov   bp, sp
    push  ax
    mov   ax, 0x0500
    int   0x10
    pop   ax
    pop   bp
    ret

cursor_rst:
    push  bp
    mov   bp, sp
    push  ax
    push  bx
    push  dx
    mov   ax, 0x0002
    xor   bx, bx
    xor   dx, dx
    int   0x10
    pop   dx
    pop   bx
    pop   ax
    pop   bp
    ret

print:
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

error:
    push  bp
    mov   bp, sp
    mov   si, [ss:bp + 6]  ; push error message again
    push  si
    mov   si, [ss:bp + 4] 
    push  si
    call  print
    cli
    hlt

reset:
    push  bp
    mov   bp, sp
    push  dx
    mov   dx, [ss:bp + 4]
    push  si
    push  ax
    push  disk_err1
    push  de1_len
    xor   si, si
resetlp:
    inc   si
    cmp   si, 0x05
    jne   resetcnt
    call  error
resetcnt:
    xor   word ax, ax
    int   0x13
    jc    resetlp
    add   sp, 4         ; discard error message
    pop   word ax
    pop   word si
    pop   dx
    pop   bp
    ret

read:
    push  bp
    mov   word bp, sp
    push  word ax
    push  word bx
    push  word cx
    mov   word bx, [ss:bp + 8]
    mov   word ax, [ss:bp + 6]
    mov   word dx, [ss:bp + 4]
    push  word si
    push  word disk_err2
    push  word de2_len
    xor   word si, si
readlp:
    inc   si
    cmp   si, 0x05
    jne   readcnt
    call  error
readcnt:
    mov   dh, 0x00
    mov   ch, 0x00
    mov   cl, 0x02
    int   0x13
    jc    readlp
    add   sp, 4
    pop   word si
    pop   word cx
    pop   word bx
    pop   word ax
    pop   bp
    ret

section .boot.rodata alloc noexec progbits nowrite
disk_err1 db "E: Disk reset failed (5 tries)",0x0D,0x0A
de1_len   equ $ - disk_err1
disk_err2 db "E: Disk read failed (5 tries)",0x0D,0x0A
de2_len   equ $ - disk_err2
