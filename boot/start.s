bits 32

extern main
global __start
section .init

__start:
    push  ebp
    mov   ebp, esp
    xor   bx, bx
    xor   ax, ax
    mov   word bx, [ebp + 4]
    mov   word ax, [ebp + 6]
    shl   eax, 4

    push  eax
    push  ebx
    xor   eax, eax
    xor   ebx, ebx

    call  main

    ; if we ever get to this point, 
    ; just give up, something went
    ; wrong.

    cli
    hlt
