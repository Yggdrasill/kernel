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

bits    16
section .boot.util alloc exec progbits nowrite

init_video:
	mov   ax, 0x03
	int   0x10

	mov   ah, 0x01
	mov   cx, 0x3F00
	int   0x10

	; page reset
	mov   ax, 0x0500
	int   0x10

	; cursor reset
	mov   ah, 0x02
	xor   bx, bx
	xor   dx, dx
	int   0x10
	ret
