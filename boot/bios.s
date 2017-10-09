%ifndef BIOS_S
%define BIOS_S

print:
    mov   bp, sp
    mov   si, [ss:bp + 2]
    push  ax

    xor   ax, ax
printlp:
    lodsb
    or    al, al
    jz    printret
    mov   ah, 0x0E
    int   0x10
    jmp   printlp
printret:
    pop   ax
    ret

error:
    mov   bp, sp
    mov   si, [ss:bp + 2]  ; push error message again
    push  si
    call  print
    cli
    hlt

reset:
    push  dx
    mov   bp, sp
    mov   dx, [ss:bp + 4]
    push  si
    push  ax
    push  disk_err1
    xor   si, si
resetlp:
    inc   si
    cmp   si, 0x05
    jne   resetcnt
    call  error
resetcnt:
    xor   ax, ax
    int   0x13
    jc    resetlp
    add   sp, 2         ; discard error message
    pop   ax
    pop   si
    pop   dx
    ret

%endif
