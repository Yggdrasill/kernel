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

bits 16

global store_bios_imr
global mask_ints
global ms_nmi_disable
global pmode_init
global rmode_trampoline_no_sret
global rmode_trampoline

global shadow_p70

section .stage15 alloc exec progbits nowrite

; Must NEVER be called in 32-bit mode.
store_bios_imr:
    push  ax

    in    al, 0x21
    mov   [bios_imr0], al
    in    al, 0xA1
    mov   [bios_imr1], al

    pop   ax
    ret

load_bios_imr:
    push  ax

    mov   al, [bios_imr0]
    out   0x21, al
    mov   al, [bios_imr1]
    out   0xA1, al

    pop   ax
    ret

; Mixed-mode functions. CAREFUL!
; Instructions must mean the same thing
; in both 16-bit and 32-bit execution!
mask_ints:
    push  ax

    mov   al, 0xFF
    out   0x21, al
    out   0xA1, al

    pop   ax
    ret

; mode_switch functions do not write
; port 0x70 shadow state.

; NOTE: Clobbers ecx, but is caller-saved.

; Mixed execution dual decoding magic
get_shadow_p70:
    xor   cx, cx
    push  strict word shadow_p70
    ; Unholy sacrificial instruction
    dec   cl
    pop   cx
    jns   short fix_shadow_p70+1
fix_shadow_p70:
    movzx ecx, cx
    jns   short load_shadow_p70+1
load_shadow_p70:
    mov   al, [ecx]
    sahf
    jnc   short ms_nmi_disable_ret
    jmp   short ms_nmi_common_ret

restore_p70:
    stc
    jmp   ms_nmi_common_entry
ms_nmi_disable:
    clc
ms_nmi_common_entry:
    push  ax
    lahf
    jmp   short get_shadow_p70
ms_nmi_disable_ret:
    or    al, 0x80
ms_nmi_common_ret:
    out   0x70, al
    pop   ax
    ret

; See ./boot/common/README.md for an explanation.
;
; End mixed-mode functions

pmode_init:
    lgdt  [gdt_info]

    ; Fix stack high bytes.
    movzx esp, sp

    ; Fix the stack pointers by converting a
    ; linear address.
    xor   ecx, ecx
    mov   cx, ss
    shl   ecx, 4
    add   esp, ecx

    mov   ecx, cr0
    or    cx, 1
    mov   cr0, ecx
    jmp   0x0008:pmode32
bits 32
pmode32:
    mov   cx, 0x10
    mov   ss, cx
    mov   es, cx
    mov   ds, cx
    mov   gs, cx
    mov   fs, cx

    lidt  [idt_info]
    ret

pmode_exit:
    lgdt  [gdt_info]

    ; Disable paging if enabled.
    mov   ecx, cr0
    btr   ecx, 31
    mov   cr0, ecx
    xor   ebx, ebx
    mov   cr3, ebx
    jmp   0x0018:pmode16
bits 16
pmode16:
    mov   bx, 0x20
    mov   ss, bx
    mov   es, bx
    mov   ds, bx
    mov   gs, bx
    mov   fs, bx

    lidt  [idt_rmode]

    and   cx, ~1
    mov   cr0, ecx
    jmp   0x0000:rmode
rmode:
    xor   cx, cx
    mov   es, cx
    mov   ds, cx
    mov   gs, cx
    mov   fs, cx

    ; This calculates a valid stack segment below
    ; 1MiB, but esp stack pointer CANNOT BE 64K ALIGNED.
    ; With this precondition in mind, the stack can
    ; otherwise live within any part of low memory.
    ; That is the memory that real mode is limited
    ; to anyway, so it is of course otherwise
    ; impossible to use the same stack.
    rol   esp, 16
    shl   sp, 12
    mov   ss, sp
    shr   esp, 16

    ; The 4-byte return address on the stack will look
    ; like 0x0000xxxx, so we can deliberately interpret
    ; the zeroed bits as a far return pointer.
    retf

segment_fix:
    ; BIOS anti-clobber
    xor   cx, cx
    mov   es, cx
    mov   ds, cx
    mov   fs, cx
    mov   gs, cx
    ret

bits 32
check_cpuid:
    xor   eax, eax
    pusha
    pushfd
    pushfd
    pushfd
    btr   dword [esp], 21
    popfd
    pushfd
    pop   eax
    bts   dword [esp], 21
    popfd
    pushfd
    pop   ecx
    popfd
    xor   ecx, eax
    popa
    setnz al
    ret

check_cr4:
    pusha
    call  check_cpuid
    jz    check_cr4_ret
    cpuid
    ; CPUID leaf 1 test for:
    ; CR4.MCE, CR4.PAE, CR4.TSD,
    ; CR4.PSE, CR4.DE, CR4.VME
    ; Definitely got CR4 if non-zero.
    test  dl, 0xDE
check_cr4_ret:
    popa
    ret

save_state:
    push  eax
    in    al, 0x21
    mov   [ecx + imr0_shadow - ms_context], al
    in    al, 0xA1
    mov   [ecx + imr1_shadow - ms_context], al
    mov   [ecx + saved_ebx - ms_context], ebx
    mov   [ecx + saved_esi - ms_context], esi
    mov   [ecx + saved_edi - ms_context], edi
    mov   [ecx + saved_ebp - ms_context], ebp
    mov   [ecx + saved_ss - ms_context], ss
    mov   [ecx + saved_es - ms_context], es
    mov   [ecx + saved_ds - ms_context], ds
    mov   [ecx + saved_fs - ms_context], fs
    mov   [ecx + saved_gs - ms_context], gs
    sgdt  [ecx + shadow_gdtr - ms_context]
    sidt  [idt_info]
    mov   eax, cr0
    mov   [ecx + saved_cr0 - ms_context], eax
    mov   eax, cr3
    mov   [ecx + saved_cr3 - ms_context], eax
    call  check_cr4
    jz    save_no_cr4
    mov   eax, cr4
    mov   [ecx + saved_cr4 - ms_context], eax
save_no_cr4:
    pop   eax
    ret

restore_state:
    push  eax
    call  check_cr4
    jz    restore_no_cr4
    mov   eax, [ecx + saved_cr4 - ms_context]
    mov   cr4, eax
restore_no_cr4:
    mov   eax, [ecx + saved_cr3 - ms_context]
    mov   cr3, eax
    mov   eax, [ecx + saved_cr0 - ms_context]
    mov   cr0, eax
    lgdt  [ecx + shadow_gdtr - ms_context]
    lidt  [idt_info]
    mov   gs, [ecx + saved_gs - ms_context]
    mov   fs, [ecx + saved_fs - ms_context]
    mov   ds, [ecx + saved_ds - ms_context]
    mov   es, [ecx + saved_es - ms_context]
    mov   ss, [ecx + saved_ss - ms_context]
    mov   ebp, [ecx + saved_ebp - ms_context]
    mov   edi, [ecx + saved_edi - ms_context]
    mov   esi, [ecx + saved_esi - ms_context]
    mov   ebx, [ecx + saved_ebx - ms_context]
    mov   al, [ecx + imr1_shadow - ms_context]
    out   0xA1, al
    mov   al, [ecx + imr0_shadow - ms_context]
    out   0x21, al
    pop   eax
    ret

rmode_trampoline_no_sret:
    pop    eax
    push   byte 0x00
    push   eax
rmode_trampoline:
    ; Save flags, cs, and clear IF
    pushfd
    push   cs
    cli
    mov   ecx, ms_context
    pop   dword [ecx + saved_cs - ms_context]
    pop   dword [ecx + saved_eflags - ms_context]
    ; Save machine state, mask all interrupts,
    ; disable NMI, then exit protected mode
    call  save_state
    call  mask_ints
    call  ms_nmi_disable
    call  pmode_exit
bits 16
    call  load_bios_imr
    call  restore_p70
    ; This may look a bit unconventional, but 
    ; popping the return address from the stack
    ; allows us to pass arguments as if calling
    ; from real mode. The return address will be
    ; pushed later.
    pop   dword [resume]
    ; Clean up the sret pointer.
    pop   dword [sret_ptr]
    ; Pop 32-bit callee address and push as 16-bit
    ; address, then call it with a tail call.
    pop   dword [callee]
    ; Push return pointer, ret into 16-bit callee
    push  word rmode_return
    push  word [callee]
    sti
    ret
rmode_return:
    ; Enter protected mode.
    cli
    call  segment_fix
    call  ms_nmi_disable
    call  mask_ints
    call  0x0000:pmode_init
bits 32
    mov   ecx, ms_context
    ; Now restore regs and machine state.
    call  restore_state
    ; Adjust stack for popped callee pointer.
    push  eax
    push  ecx
    ; Deal with Sret pointer and return data.
    mov   ecx, [ecx + sret_ptr - ms_context]
    test  ecx, ecx
    jz    skip_sret
    mov   [ecx], eax
    xchg  eax, ecx
skip_sret:
    call  restore_p70
    ; Push return path
    pop   ecx
    push  dword [ecx + saved_eflags - ms_context]
    push  dword [ecx + saved_cs - ms_context]
    push  dword [ecx + resume - ms_context]
    iretd

section .stage15.ms alloc noexec progbits write
gdt:
null_gdt    times 8 db 0
code_32     db 0xFF,0xFF,0x00,0x00,0x00,0x9B,0xCF,0x00
data_32     db 0xFF,0xFF,0x00,0x00,0x00,0x93,0xCF,0x00
code_16     db 0xFF,0xFF,0x00,0x00,0x00,0x9B,0x00,0x00
data_16     db 0xFF,0xFF,0x00,0x00,0x00,0x93,0x00,0x00
gdt_len     equ $ - gdt

idt_rmode:
; Real mode interrupt vectors are 4 bytes in size:
;   256 entries * 4 bytes - 1 = 0x03FF
; IVT exists at 0x0000
idt_rsize    dw 0x03FF
idt_rptr     dd 0x0000

gdt_info:
gdt_size    dw  gdt_len - 1
gdt_ptr     dd  gdt

idt_info:
idt_size     dw 0
idt_ptr      dd 0

shadow_p70   db 0x80

section .stage15.bss bss alloc noexec nobits write
ms_context:
imr0_shadow:  resb 1
imr1_shadow:  resb 1
bios_imr0:    resb 1
bios_imr1:    resb 1

shadow_gdtr:
sgdt_size     resw 1
sgdt_ptr      resd 1

saved_eflags: resd 1
saved_cs:     resd 1
saved_cr0:    resd 1
saved_cr3:    resd 1
saved_cr4:    resd 1
saved_ebx:    resd 1
saved_esi:    resd 1
saved_edi:    resd 1
saved_ebp:    resd 1
resume:       resd 1
callee:       resd 1
sret_ptr:     resd 1
saved_ss:     resw 1
saved_es:     resw 1
saved_ds:     resw 1
saved_fs:     resw 1
saved_gs:     resw 1
