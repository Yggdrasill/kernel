%ifndef VGA_S
%define VGA_S

init_video:
    push  ax
    push  cx

    mov   ah, 0x00
    mov   al, 0x03
    int   0x10

    xor   ax, ax
    xor   cx, cx
    mov   ah, 0x01
    mov   ch, 0x3F
    int   0x10

    pop   cx
    pop   ax

    ret

%endif
