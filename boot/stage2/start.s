bits 32

section ._init alloc exec progbits
global __start

__start:
    push  ebp
    mov   ebp, esp
    xor   ebx, eax
    xor   eax, ebx
    mov   word ax, [ebp + 6]
    mov   word bx, [ebp + 4]
    shl   eax, 4
    add   ebx, eax

    mov   esp, 0x7FFF0
    mov   ebp, 0x7FFF0
    
    push  ebx
    push  eax
    xor   eax, eax
    xor   ebx, ebx

    call  main

    ; if we ever get to this point, 
    ; just give up, something went
    ; wrong.

    cli
    hlt

section .text
extern main
