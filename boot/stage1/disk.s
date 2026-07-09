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

extern __bios_error

global disk_geometry
global reset
global read

global drive

bits    16
section .boot.util alloc exec progbits nowrite

disk_geometry:
	mov   ah, 0x08
	push  ds
	int   0x13
	pop   ds
	jnc   geometry_done
geometry_e:
	push  dword de3_len
	push  dword disk_err3
	call  __bios_error
geometry_done:
	mov   al, ch
	mov   ah, cl
	shr   ah, 6
	and   cx, 0x3F
	mov   [disk_cylinders], ax
	mov   [disk_sectors],   cl
	mov   [disk_heads],     dh
	ret

reset:
	pusha
	mov   cx, 0x05
resetlp:
	xor   ax, ax
	push  es
	push  ds
	pusha
	int   0x13
	popa
	pop   ds
	pop   es
	jnc   reset_done
	loop  resetlp
reset_e:
	push  dword de1_len
	push  dword disk_err1
	call  __bios_error
reset_done:
	popa
	ret

read:
	xor   di, di
	or    si, si
	jz    read_done
read_sector:
	push  es
	push  ds
	pusha
	mov   ax, 0x0201
	int   0x13
	popa
	pop   ds
	pop   es
	jc    read_recover
read_success:
	add   bx, 0x200
	dec   si
	jz    read_done
	xor   di, di
	mov   al, cl
	and   al, 0x3F
	and   cl, 0xC0
	inc   al
	cmp   al, [disk_sectors]
	ja    read_next_head
	or    cl, al
	jmp   read_sector
read_next_head:
	inc   cl
	inc   dh
	cmp   dh, [disk_heads]
	jbe   read_sector
	xor   dh, dh
	mov   al, ch
	mov   ah, cl
	shr   ah, 6
	inc   ax
	mov   ch, al
	mov   cl, ah
	shl   cl, 6
	inc   cl
	cmp   ax, [disk_cylinders]
	ja    read_e
	jmp   read_sector
read_recover:
	call  reset
	inc   di
	cmp   di, 0x05
	jl    read_sector
read_e:
	push  dword de2_len
	push  dword disk_err2
	call  __bios_error
read_done:
	ret

disk_cylinders		dw 0
disk_sectors		db 0
disk_heads			db 0
drive               db 0

section .boot.rodata alloc noexec progbits nowrite
disk_err1    db "E: Disk reset",0x0D,0x0A
de1_len      equ $ - disk_err1
disk_err2    db "E: Disk read",0x0D,0x0A
de2_len      equ $ - disk_err2
disk_err3    db "E: Disk geometry",0x0D,0x0A
de3_len      equ $ - disk_err3
