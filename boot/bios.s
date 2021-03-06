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
    mov   bp, sp
    mov   si, [ss:bp + 2]
    push  ax

    xor   ax, ax
printlp:
    lodsb
    or    al, al
    jz    printret
    mov   ah, 0x0E
    int   0x10
    jmp   printlp
printret:
    pop   ax
    ret

error:
    mov   bp, sp
    mov   si, [ss:bp + 2]  ; push error message again
    push  si
    call  print
    cli
    hlt

reset:
    push  dx
    mov   bp, sp
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
    xor   ax, ax
    int   0x13
    jc    resetlp
    add   sp, 2         ; discard error message
    pop   ax
    pop   si
    pop   dx
    ret

read:
    push  ax
    push  bx
    push  cx
    mov   bp, sp
    mov   bx, [ss:bp + 14]
    mov   es, bx
    mov   bx, [ss:bp + 12]
    mov   ax, [ss:bp + 10]
    mov   dx, [ss:bp + 8]
    push  si
    push  disk_err2
    xor   si, si
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
    pop   si
    pop   cx
    pop   bx
    pop   ax
    ret

disk_err1 db "Error: Couldn't reset drive in less than 5 tries",0x0D,0x0A,0
disk_err2 db "Error: Couldn't read drive in less than 5 tries",0x0D,0x0A,0

%endif
