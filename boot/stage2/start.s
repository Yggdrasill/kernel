extern __STACK_BASE_LOCATION

extern __bios_error

section ._init alloc exec progbits nowrite
global __start

__start:
bits 16
    xor    ax, ax
    inc    ax
    shl    ax, 16
    bt     ax, 16
    jc     continue_32
    push   dword exec_len
    push   dword exec_err
    call   __bios_error
bits 32
continue_32:
    push  dword 0x02
    popfd

    mov   esp, __STACK_BASE_LOCATION
    mov   ebp, __STACK_BASE_LOCATION
    call  main
    ; if we ever get to this point, 
    ; just give up, something went
    ; wrong.
    cli
hltlp:
    hlt
    jmp   hltlp

exec_err db "E: Cannot execute stage 2 in 16-bit mode!",0x0D,0x0A
exec_len equ $ - exec_err

section .text
extern main
