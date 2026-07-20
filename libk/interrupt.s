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

bits 32
section .text

global nmi_enable
global nmi_disable

extern shadow_p70

nmi_enable:
    push ebp
    mov  ebp, esp
    push eax
    mov  al, [shadow_p70]
    and  al, 0x7F
    out  0x70, al
    mov  [shadow_p70], al
    pop  eax
    pop  ebp
    ret

nmi_disable:
    push ebp
    mov  ebp, esp
    push eax
    mov  al, [shadow_p70]
    or   al, 0x80
    out  0x70, al
    mov  [shadow_p70], al
    pop  eax
    pop  ebp
    ret
