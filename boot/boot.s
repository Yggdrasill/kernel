bits   16

cli
mov   ax, 0x7000
mov   ss, ax
mov   sp, 0xFFFF

mov   ax, 0x07E0
mov   es, ax
mov   ax, 0x07C0
mov   ds, ax
xor   si, si
xor   di, di

mov   cx, 0x0200
rep   movsb

mov   ax, 0x07E0
mov   ds, ax
mov   es, ax
xor   di, di
xor   si, si

sti

jmp   0x07E0:boot

%include "bios.s"

boot:
    mov   [drive], dl

    xor   dx, dx
    mov   dl, [drive]
    push  dx
    call  reset
    mov   sp, 0xFFFF    ; flush stack

    xor   dx, dx
    mov   dl, [drive]
    push  word 0x07E0
    push  word 0x0200
    push  word 0x023C   ; read a ridiculous amount of sectors
    push  dx
    call  read
    mov   sp, 0xFFFF

    cli
    hlt

drive     db 0

times     446 - ($ - $$) db 0

part0     times 16 db 0
part1     times 16 db 0
part2     times 16 db 0
part3     times 16 db 0

dw        0xAA55
