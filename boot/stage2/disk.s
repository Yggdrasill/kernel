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

global __bios_edd_available

%include "s2_generated.s"

bits 16
section .text

__bios_edd_available:
    push  bp
    mov   bp, sp
    xor   eax, eax
    xor   ecx, ecx
    mov   ah, 0x41
    mov   bx, 0x55AA
    mov   dl, byte [bp + 4]
    pop   bp
    int   0x13
    jnc   avail_verify
    mov   ah, -1 
    jc    edd_avail_ret
avail_verify:
    cmp   bx, 0xAA55
    je    edd_avail_ret
    mov   ah, -2
edd_avail_ret:
    ; Success return is either
    ; 0x10, 0x20, or 0x30. Safe
    ; to sign-extend for error
    ; codes.
    xchg  al, ah
    cbw
    shl   ecx, 8
    or    eax, ecx
    ret
