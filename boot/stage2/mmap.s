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

extern __MMAP_BASE_ADDR

global __bios_mmap
extern __bios_error

%include "mmap_generated.s"

bits    16
section .text

__bios_mmap:
	push  dword ebp
	mov   ebp, esp 
	push  word es
	push  word ds

	mov   edx, [bp + 6]
	mov   eax, edx
	shr   eax, 4
	mov   ds, ax
	and   edx, 0x0F
	mov   eax, [edx + E820_INFO_BASE]
	mov   edi, eax
	shr   eax, 4
	mov   es, ax
	and   edi, 0x0F
	mov   eax, [edx + E820_INFO_NR]
	mul   eax, E820_ENTRY_SIZE
	add   edi, eax
	mov   eax, [edx + E820_INFO_MAX]
	mul   eax, E820_ENTRY_SIZE
	mov   esi, eax
	xor   ebx, ebx
mmap_loop:
	mov   eax, edi
	sub   eax, esi
	js    mmap_e820
	mov   ecx, -4
	jmp   mmap_done
mmap_e820:
	; clear ACPI 3.0 attribute field if BIOS doesn't fill in
	mov   dword [es:edi+0x14], 0x00 
	mov   eax, 0x0000E820
	mov   ecx, 0x00000018
	mov   edx, 0x534D4150
	push  edi
	push  esi
	push  es
	push  ds
	int   0x15
	pop   ds
	pop   es
	pop   esi
	pop   edi
	jc    mmap_recover

	cmp   eax, 0x534D4150
	je    check_size
	mov   ecx, -2
	jmp   mmap_done
check_size:
	cmp   ecx, E820_ENTRY_SIZE - 4
	je    mmap_continue
	cmp   ecx, E820_ENTRY_SIZE
	je    mmap_continue
	mov   ecx, -3
	jmp   mmap_done
mmap_continue:
	add   edi, E820_ENTRY_SIZE
	cmp   ebx, 0x00
	jnz   mmap_loop
	xor   ecx, ecx
mmap_done:
	xor   edx, edx
	mov   eax, edi
	mov   ebx, E820_ENTRY_SIZE
	div   ebx
	mov   edx, [bp + 6]
	and   edx, 0x0F
	mov   [edx + E820_INFO_NR], eax

	mov   eax, ecx
	pop   word ds
	pop   word es
	pop   dword ebp
	ret

mmap_recover:
	; Return value if successful
	xor   ecx, ecx
	mov   edx, [bp + 6]
	and   edx, 0x0F
	mov   eax, [edx + E820_INFO_NR]
	mul   eax, E820_ENTRY_SIZE
	cmp   edi, eax
	jne   mmap_done
	; E820 not supported
	mov   ecx, -1
	jmp   mmap_done
