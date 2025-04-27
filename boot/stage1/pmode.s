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

global gdt_install
global idt_install
global pmode_init
global pmode_exit
extern mask_ints

bits    16
section .stage15 alloc exec progbits nowrite

gdt_install:
    push  bp
    mov   bp, sp
    push  di
    mov   di, gdt_info
    lgdt  [es:di]
    pop   di
    pop   bp
    ret

idt_install:
    push  bp
    mov   bp, sp
    push  di
    mov   di, idt_info
    lidt  [es:di]
    pop   di
    pop   bp
    ret

pmode_init:
    ; Reserve space for return pointer.
    sub   esp, 2

    push  ebp
    mov   bp, sp
    push  eax

    call  mask_ints

    cli

    call  idt_install
    call  gdt_install

    ; Fix the stack pointers by converting a
    ; linear address.

    xor   eax, eax
    mov   eax, ss
    mov   [stack_seg], ax
    shl   eax, 4
    add   eax, esp
    mov   esp, eax

    ; Set the protected mode bit.

    mov   eax, cr0
    or    eax, 1
    mov   cr0, eax

    ; Initialize segment registers to use the GDT.
    ; There should be no more 16 bit or real mode
    ; code executed after this point, except for
    ; these simple mov instructions and a jump.

    mov   ax, 0x0010
    mov   ss, ax
    mov   es, ax
    mov   ds, ax
    mov   gs, ax
    mov   fs, ax

    ; Initialise code segment to use the GDT.
    ; After this jmp we are in 32-bit pmode.

    jmp   0x0008:pmode32
bits 32
pmode32:

    pop   eax
    pop   ebp

    ; ebp needs to be fixed the same way that
    ; esp was earlier.

    push  eax
    movzx eax, word [stack_seg]
    shl   eax, 4
    add   eax, ebp
    mov   ebp, eax

    ; The return pointer was pushed as a 2-byte
    ; value, since we were called from 16-bit
    ; code. We have preallocated space for the
    ; return pointer at the beginning of this
    ; function, and now need to push it as 4 bytes.

    ; Read from esp + 6 to account for extra 2 bytes
    ; reserved, and the eax push.
    
    movzx eax, word [esp + 6]
    mov   [return], eax
    pop   eax
    ; Overwrite old value
    add   esp, 4
    push  dword [return]

    sti
    ret

pmode_exit:
    push  ebp
    mov   ebp, esp
    push  eax
    push  edi

    jmp   0x0018:pmode16
pmode16:

    cli
    mov   ax, 0x20
    mov   ss, ax
    mov   es, ax
    mov   ds, ax
    mov   gs, ax
    mov   fs, ax

    mov   eax, cr0 
    and   eax, ~1
    mov   cr0, eax

    jmp   0x0000:rmode
rmode:
bits 16

    xor   ax, ax
    mov   es, ax
    mov   ds, ax
    mov   gs, ax
    mov   fs, ax
    mov   eax, ebp
    shr   eax, 4
    and   eax, 0xF000
    mov   ss, ax


    lidt  [idt_rmode]

    ; Clean up higher bits in esp, as they can mess up
    ; the stack. This is because i386 has 32-bit
    ; register extensions.
    and   esp, 0xFFFF

    pop   edi
    pop   eax

    ;Clean up ebp in the same way, and for the same reason.
    pop   ebp
    and   ebp, 0xFFFF

    sti
    ret


section .data
gdt_info:
gdt_size  dw  gdt_len - 1
gdt_ptr   dd  gdt

gdt:
null_gdt  times 8 db 0
code_32   db 0xFF,0xFF,0x00,0x00,0x00,0x9B,0xFC,0x00
data_32   db 0xFF,0xFF,0x00,0x00,0x00,0x93,0xFC,0x00
code_16   db 0xFF,0xFF,0x00,0x00,0x00,0x9B,0x0F,0x00
data_16   db 0xFF,0xFF,0x00,0x00,0x00,0x93,0x0F,0x00
gdt_len   equ $ - gdt

idt_rmode:
; Real mode interrupt vectors are 4 bytes in size:
;   256 entries * 4 bytes - 1 = 0x03FF
; IVT exists at 0x0000
idt_rsize dw 0x03FF
idt_rptr  dd 0x0000

idt_info:
idt_size  dw 0
idt_ptr   dd 0

section .bss
return    dd 0
stack_seg dw 0
