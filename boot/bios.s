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

; This file contains a few BIOS interrupt call wrappers. The print function uses
; the BIOS to put text on the screen, for example. The error function calls
; print and then halts.
;
; Other functions may need a little bit of an explanation. The reset function uses
; BIOS interrupt call int 0x13 ah=0x00 dl=drive to reset the disk drive, whether
; it be a floppy or a hard drive, which forces the drive to place its head at the
; first track, and makes the drive execute the next command as if it was in its
; initial state. This is done before a read operation just to be sure.
;
; The read function uses a relatively complex BIOS function call in which quite
; a few registers have to be set. It uses the int 0x13 ah=0x02 dl=drive BIOS
; interrupt call. For whatever reason, instead of the typical es:di combination
; for a memory pointer, this particular call requires es:bx to be the buffer
; pointer. The dh register tells the BIOS what head to read with, the al
; register is the number of sectors to read into memory. The cx register
; specifies where to read from, where ch is the track number and cl is the
; sector number (both counting from zero of course).

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
