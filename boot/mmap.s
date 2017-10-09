%ifndef MMAP_S
%define MMAP_S

%include "bios.s"

mmap:
    push  eax
    push  ebx
    push  ecx
    push  edx
    push  es
    push  di

    mov   ax, 0x6000
    mov   es, ax
    mov   di, 0x0000

    xor   eax, eax
    push  eax

    xor   ebx, ebx
loop:
    mov   eax, 0x0000E820
    mov   ecx, 0x00000014
    mov   edx, 0x534D4150

    int   0x15
    jnc   mmapcnt1
    push  mmap_err1
    call  error

mmapcnt1:
    cmp   eax, 0x534D4150
    je    mmapcnt2
    push  mmap_err2
    call  error

mmapcnt2:
    cmp   ebx, 0x00
    je    mmap_done

    cmp   ecx, 0x14
    je    mmapcnt3
    push  mmap_err2
    call  error

mmapcnt3:
    add   di, 0x14
    pop   eax
    inc   eax
    push  eax
    jmp   loop

mmap_done:
    mov   [mmap_seg], es
    mov   [mmap_off], di

    pop   eax
    pop   di
    pop   es
    pop   edx
    pop   ecx
    pop   ebx
    pop   eax

    ret

mmap_err1 db "Error: BIOS does not support int 0x15 0xE820",0x0D,0x0A,0
mmap_err2 db "Error: mmap failed, BIOS gave malformed request",0x0D,0x0A,0

%endif
