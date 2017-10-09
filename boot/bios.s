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

%endif
