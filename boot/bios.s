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

%ifndef BIOS_S
%define BIOS_S

print:
    push  bp
    mov   bp, sp
    mov   si, [ss:bp + 4]
    push  ax
    xor   ax, ax
printlp:
    lodsb
    or   al, al
    jz   printret
    mov  ah, 0x0E
    int  0x10
    jmp  printlp
printret:
    pop  ax
    pop  bp 
    ret

error:
    push  bp
    mov   bp, sp
    mov   si, [ss:bp + 2]  ; push error message again
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
    add   sp, 2         ; discard error message
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
    add   sp, 2
    pop   word si
    pop   word cx
    pop   word bx
    pop   word ax
    pop   bp
    ret

disk_err1 db "Error: Couldn't reset drive in less than 5 tries",0x0D,0x0A,0
disk_err2 db "Error: Couldn't read drive in less than 5 tries",0x0D,0x0A,0

%endif
