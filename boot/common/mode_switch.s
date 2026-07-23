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
    jmp   short restore_p70_ret

ms_nmi_disable:
    push  ax
    clc
    lahf
    jmp   short get_shadow_p70
ms_nmi_disable_ret:
    or    al, 0x80
    out   0x70, al
    pop   ax
    ret

restore_p70:
    push  ax
    stc
    lahf
    jmp   short get_shadow_p70
restore_p70_ret:
    out   0x70, al
    pop   ax
    ret

; Explanation of the above:
; ms_nmi_disable and restore_p70 use flag CF
; to select the return point, then jump to
; get_shadow_p70:
; full (e)cx clear, mode dependent
; xor cx, cx
;
; Now, the next instructions:
; push strict word shadow_p70
; dec  cl
; Magic aside, they encode 16-bit code:
; 0x68iw - 3 bytes long, iw = 16-bit address
; 0xFEC9 - 2 bytes long
;
; In 32-bit mode the decoder interprets:
; 0x68iwFEC9 - 5 bytes long, iw = 16-bit address
;
; In 16-bit mode the decoder interprets:
; 0x68iw - 3 bytes long, iw = 16-bit address
; 0xFEC9 - 2 bytes long, dec cl
;
; The sign flag now depends on execution mode.
; In other words, the sign flag is not set in
; 32-bit mode because the earlier xor cleared
; it. In 16-bit mode the sign flag IS set, as
; zero was decremented by one.
;
; This whole setup now allows us to use the
; jns instruction to discriminate mode, which
; is useful. The following movzx instruction
; fulfills two purposes: clean up ecx in 32-bit
; mode, as it contains garbage from dec cl, and
; clean it up in 16-bit mode in case the high
; half of ecx contained data not cleared by xor.
;
; However, the operand override prefix changes
; the meaning of movzx ecx, cx in 32-bit mode,
; meaning the processor will decode as mov cx, cx.
; The prefix byte is skipped with the jns. The
; same trick is used in the following mov, as the
; instruction decodes to different dereferenced
; registers between modes.
;
; End mixed-mode functions

pmode_init:
    ; Fix stack high bytes and reserve
    ; space for return pointer.
    and   ebp, 0xFFFF
    and   esp, 0xFFFF
    sub   esp, 2

    push  ebp
    mov   bp, sp
    push  eax

    lgdt  [gdt_info]
    lidt  [idt_info]

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

    ; Initialise code segment to use the GDT.
    ; After this jmp we are in 32-bit pmode.

    jmp   0x0008:pmode32
bits 32
pmode32:

    ; Initialize segment registers to use the GDT.

    mov   ax, 0x0010
    mov   ss, ax
    mov   es, ax
    mov   ds, ax
    mov   gs, ax
    mov   fs, ax

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

    ret

pmode_exit:
    push  ebp
    mov   ebp, esp
    push  eax
    push  edi

    lgdt  [gdt_info]

    jmp   0x0018:pmode16
bits 16
pmode16:

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

    ; Initialize segment registers for real mode.
    xor   ax, ax
    mov   es, ax
    mov   ds, ax
    mov   gs, ax
    mov   fs, ax

    ; This calculates a valid stack segment below
    ; 1MiB, but ebp stack base CANNOT BE 64K ALIGNED.
    ; With this precondition in mind, the stack can
    ; otherwise live within any part of low memory.
    ; That is the memory that real mode is limited
    ; to anyway, so it is of course otherwise
    ; impossible to use the same stack.
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

    ; Now fix the return pointer on the stack and realign,
    ; since this function was entered with a 4-byte return
    ; pointer and will exit with a 2-byte one.
    push  eax
    mov   eax, dword [esp + 4]
    mov   [return], eax
    pop   eax
    add   esp, 4
    push  word [return]

    ret

segment_fix:
    ; BIOS anti-clobber
    xor   cx, cx
    mov   es, cx
    mov   ds, cx
    mov   fs, cx
    mov   gs, cx
    ret

bits 32
save_state:
    push  eax
    in    al, 0x21
    mov   [imr0_shadow], al
    in    al, 0xA1
    mov   [imr1_shadow], al
    mov   [saved_ebx], ebx
    mov   [saved_esi], esi
    mov   [saved_edi], edi
    mov   [saved_ebp], ebp
    mov   [saved_ss], ss
    mov   [saved_es], es
    mov   [saved_ds], ds
    mov   [saved_fs], fs
    mov   [saved_gs], gs
    sgdt  [shadow_gdtr]
    sidt  [idt_info]
    pop   eax
    ret

restore_state:
    push  eax
    lgdt  [shadow_gdtr]
    lidt  [idt_info]
    mov   gs, [saved_gs]
    mov   fs, [saved_fs]
    mov   ds, [saved_ds]
    mov   es, [saved_es]
    mov   ss, [saved_ss]
    mov   ebp, [saved_ebp]
    mov   edi, [saved_edi]
    mov   esi, [saved_esi]
    mov   ebx, [saved_ebx]
    mov   al, [imr1_shadow]
    out   0xA1, al
    mov   al, [imr0_shadow]
    out   0x21, al
    pop   eax
    ret

rmode_trampoline_no_sret:
    pop    dword [resume]
    push   dword 0x00
    push   dword [resume]
rmode_trampoline:
    ; Save flags, cs, and clear DF/IF
    pushfd
    push   cs
    cli
    cld
    pop    dword [saved_cs]
    pop    dword [saved_eflags]
    ; Save machine state, mask all interrupts,
    ; disable NMI, then exit protected mode
    call   save_state
    call   mask_ints
    call   ms_nmi_disable
    call   pmode_exit
bits 16
    call   load_bios_imr
    call   restore_p70
    ; This may look a bit unconventional, but 
    ; popping the return address from the stack
    ; allows us to pass arguments as if calling
    ; from real mode. The return address will be
    ; pushed later.
    pop    dword [resume]
    ; Clean up the sret pointer.
    pop    dword [sret_ptr]
    ; Pop 32-bit callee address and push as 16-bit
    ; address, then call it with a tail call.
    pop    dword [callee]
    ; Push return pointer, ret into 16-bit callee
    push   rmode_return
    push   word [callee]
    sti
    ret
rmode_return:
    ; Enter protected mode.
    cli
    call   segment_fix
    call   ms_nmi_disable
    call   mask_ints
    call   pmode_init
bits 32
    ; Allocate space for popped callee pointer.
    sub    esp, 4
    mov    ebx, [sret_ptr]
    cmp    ebx, dword 0x00
    je     skip_sret
    mov    [ebx], eax
    mov    eax, ebx
skip_sret:
    ; Now restore regs and machine state.
    call   restore_state
    call   restore_p70
    ; Push return path
    push   dword [saved_eflags]
    push   dword [saved_cs]
    push   dword [resume]
    iretd

section .stage15.data
shadow_p70   db 0x00

gdt_info:
gdt_size    dw  gdt_len - 1
gdt_ptr     dd  gdt

gdt:
null_gdt    times 8 db 0
code_32     db 0xFF,0xFF,0x00,0x00,0x00,0x9B,0xCF,0x00
data_32     db 0xFF,0xFF,0x00,0x00,0x00,0x93,0xCF,0x00
code_16     db 0xFF,0xFF,0x00,0x00,0x00,0x9B,0x0F,0x00
data_16     db 0xFF,0xFF,0x00,0x00,0x00,0x93,0x0F,0x00
gdt_len     equ $ - gdt

idt_rmode:
; Real mode interrupt vectors are 4 bytes in size:
;   256 entries * 4 bytes - 1 = 0x03FF
; IVT exists at 0x0000
idt_rsize    dw 0x03FF
idt_rptr     dd 0x0000

idt_info:
idt_size     dw 0
idt_ptr      dd 0

section .stage15.bss bss alloc noexec nobits write
imr0_shadow:  resb 1
imr1_shadow:  resb 1
bios_imr0:    resb 1
bios_imr1:    resb 1

shadow_gdtr:
sgdt_size     resw 1
sgdt_ptr      resd 1

pmode_context:
saved_eflags: resd 1
saved_ebx:    resd 1
saved_esi:    resd 1
saved_edi:    resd 1
saved_ebp:    resd 1
saved_cs:     resd 1
return:       resd 1
stack_seg:    resd 1
resume:       resd 1
callee:       resd 1
sret_ptr:     resd 1
saved_ss:     resw 1
saved_es:     resw 1
saved_ds:     resw 1
saved_fs:     resw 1
saved_gs:     resw 1
