bits 32

section ._init alloc exec progbits nowrite
global __start

__start:
    push  ebp
    mov   ebp, esp

    mov   esp, 0x7FFF0
    mov   ebp, 0x7FFF0
    
    call  main

    ; if we ever get to this point, 
    ; just give up, something went
    ; wrong.

    cli
    hlt

section .text
extern main
