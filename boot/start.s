bits 32

extern main
global __start
section .init

__start:
    push  ebp
    mov   ebp, esp
    xor   ebx, ebx
    xor   eax, eax
    mov   word bx, [ebp + 4]
    mov   word ax, [ebp + 6]
    shl   eax, 4
    add   ebx, eax

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
