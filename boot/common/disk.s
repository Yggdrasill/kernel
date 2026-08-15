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

global __chs_geometry
global __disk_reset
global __chs_read

global CHS_FLOPPY_CYLINDERS
global CHS_FLOPPY_HEADS
global CHS_FLOPPY_SECTORS

; Some compile target thing should
; reconfigure these for other floppy
; types. Floppy geometry is awkward,
; and the BPB can't be trusted because
; BIOSes will just clobber it.
CHS_FLOPPY_CYLINDERS equ 0x4F
CHS_FLOPPY_HEADS     equ 0x01
CHS_FLOPPY_SECTORS   equ 0x12

bits    16
section .boot.util alloc exec progbits nowrite

int13:
    push  es
    push  ds
    int   0x13
    pop   ds
    pop   es
int13_hook:
    jmp   strict near int13_ret
int13_ret:
    ret

disk_geometry:
    mov   ah, 0x08
    call  int13
    jnc   geometry_done
geometry_e:
    push  dword de3_len
    push  dword disk_err3
    call  __bios_error
geometry_done:
    mov   al, ch
    mov   ah, cl
    shr   ah, 6
    and   cl, 0x3F
geometry_write:
    mov   [disk_cylinders], ax
    mov   [disk_heads],     dh
    mov   [disk_sectors],   cl
    ret

reset:
    mov   cx, 0x05
resetlp:
    pusha
    xor   ax, ax
    call  int13
    popa
    jnc   reset_done
    loop  resetlp
reset_e:
    push  dword de1_len
    push  dword disk_err1
    call  __bios_error
reset_done:
    ret

read:
    jz    read_done
read_sector:
    mov   di, 5
read_try:
    pusha
    mov   ax, 0x0201
    call  int13
    popa
    jc    read_recover
read_success:
    add   bh, 2
    jnc   read_continue
    mov   ax, es
    add   ax, 0x1000
    mov   es, ax
read_continue:
    dec   esi
    jz    read_done
    mov   al, cl
    and   al, 0x3F
    cmp   al, [disk_sectors]
    jae   read_next_head
    inc   cl
    jmp   read_sector
read_next_head:
    and   cl, 0xC0
    inc   cl
    cmp   dh, [disk_heads]
    je    read_next_cylinder
    inc   dh
    jmp   read_sector
read_next_cylinder:
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
    jbe   read_sector
read_error:
    jmp   strict near read_e
read_done:
    ret

read_recover:
    pusha
    call  reset
    popa
    dec   di
    jnz   read_try
read_e:
    push  dword de2_len
    push  dword disk_err2
    call  __bios_error

disk_cylinders dw CHS_FLOPPY_CYLINDERS
disk_heads     db CHS_FLOPPY_HEADS
disk_sectors   db CHS_FLOPPY_SECTORS

section .boot.rodata alloc noexec progbits nowrite
disk_err1    db "E: Disk reset",0x0D,0x0A
de1_len      equ $ - disk_err1
disk_err2    db "E: Disk read",0x0D,0x0A
de2_len      equ $ - disk_err2
disk_err3    db "E: Disk geometry",0x0D,0x0A
de3_len      equ $ - disk_err3

section .stage15 alloc exec progbits nowrite
%include "s1_generated.s"

; We want to hook int 0x13 so we
; can manage state manually. The
; __bios_error call hangs the
; machine, so we don't want to
; end up there.
hook_install:
    mov   si, int13_hook
    sub   ax, int13_ret
    mov   [ds:si + 1], ax
    ret

addr_calc:
    push  ax
    mov   eax, edi
    shr   eax, 4
    mov   es, ax
    and   di, 0x0F
    pop   ax
    ret

; uint32_t __chs_geometry(struct disk_info *disk, uint8_t drive);
__chs_geometry:
    push  bp
    mov   bp, sp
    mov   ax, geometry_hook
    call  hook_install
    mov   dl, [ss:bp + 8]
    mov   edi, [ss:bp + 4]
    call  addr_calc
    pop   bp
    push  di
    call  disk_geometry
geometry_hook:
    ; Get rid of two return pointers
    ; without touching flags.
    pop   esi
    pop   di
    mov   [status], ah
    jc    hook_exit
    call  geometry_done
geometry_save:
    mov   [es:di + ABI_DISK_CYLINDERS], ax
    mov   [es:di + ABI_DISK_HEADS],     dh
    mov   [es:di + ABI_DISK_SECTORS],   cl
    mov   [es:di + ABI_DISK_DRIVES],    dl
    jmp   hook_exit

; uint32_t __disk_reset(uint8_t drive);
__disk_reset:
    push  bp
    mov   bp, sp
    mov   ax, reset_hook
    call  hook_install
    mov   dl, [ss:bp + 4]
    call  reset
reset_hook:
    ; Get rid of a return pointer,
    ; once again without touching
    ; flags. Discard one pusha
    ; frame, store ah, then popa
    ; and leave the stack frame.
    pop   bp
    mov   [status], ah
    popa
    leave
hook_exit:
    movzx eax, byte [status]
    ret

read_hook:
    ; More stack shenanigans.
    pop   bp
    ; ECC corrected, compare
    ; without touching flags.
    movzx ecx, ah
    lea   ecx, [ecx - 0x11]
    jcxz  hook_ecc
    jmp   hook_save
hook_ecc:
    ; Now clear ah and CF.
    xor   ah, ah
hook_save:
    mov   [status], ah
    popa
jmp_success:
    jnc   read_success
    jmp   read_hook_ret
invalid_geometry:
    push  chs_read_return
read_error_hook:
    ; Reference RBIL int 0x13 ah=0x01
    ; 0x04: sector not found/read error
    mov   [status], byte 4
read_hook_ret:
    ret

; int32_t __chs_read(
;    struct disk_info *disk,
;    char *buffer,
;    size_t blocks,
;    uint32_t lba,
;    uint8_t drive);
__chs_read:
    push  bp
    mov   bp, sp
    mov   ax, read_hook
    call  hook_install
    ; Hook the second __bios_error
    ; branch in the read function.
    mov   ax, read_error_hook
    mov   si, read_error
    sub   ax, read_done
    mov   [si + 1], ax

    ; Load disk geometry and write it.
    mov   edi, [ss:bp + 0x04]
    call  addr_calc
    mov   ax, [es:di + ABI_DISK_CYLINDERS]
    mov   dh, [es:di + ABI_DISK_HEADS]
    mov   cl, [es:di + ABI_DISK_SECTORS]
    ; Load buffer pointer.
    mov   edi, [ss:bp + 0x08]
    call  addr_calc
    xchg  di, bx
    ; Bail if the geometry is invalid.
    ; That is, sectors = 0.
    test  cl, cl
    jz    invalid_geometry
chs_read_continue:
    call  geometry_write
    ; Now calculate CHS from LBA.
    mov   eax, [ss:bp + 0x10]
    push  dx
    movzx edi, cl
    xor   edx, edx
    div   edi
    xchg  cx, dx
    inc   cl
    pop   dx
    movzx edi, dh
    inc   di
    xor   edx, edx
    div   edi
    mov   ch, al
    and   ah, 0x3
    shl   ah, 6
    or    cl, ah
    mov   dh, dl

    ; Finally load blocks to read
    ; and the drive number.
    mov   dl, [ss:bp + 0x14]
    ; read detects zero-size read
    ; with zero flag, due to space
    ; limitations in the boot sector
    mov   byte [status], 0
    xor   esi, esi
    add   esi, [ss:bp + 0x0C]
    call  read
chs_read_return:
    ; Return format:
    ; ah = 0 on success
    ; ah in bits 20-28
    ; bytes read in bits 0-19
    xor   eax, eax
    mov   ax, es
    shl   eax, 4
    movzx ebx, bx
    add   eax, ebx
    sub   eax, [ss:bp + 0x08]
    movzx ebx, byte [status]
    shl   ebx, 20
    or    eax, ebx
    pop   bp
    ret

section .stage15.data alloc noexec progbits write
status db 0
