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

boot:
    cli
    hlt
