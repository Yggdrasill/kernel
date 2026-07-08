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

global init_video
global vga_page_rst
global cursor_rst

bits    16
section .boot.util alloc exec progbits nowrite

init_video:
	push  bp
	mov   bp, sp
	push  ax
	push  cx

	mov   ah, 0x00
	mov   al, 0x03
	int   0x10

	xor   ax, ax
	xor   cx, cx
	mov   ah, 0x01
	mov   ch, 0x3F
	int   0x10

	pop   cx
	pop   ax
	pop   bp

	ret

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
