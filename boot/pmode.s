%ifndef PMODE_S
%define PMODE_S

gdt_install:
    lgdt  [es:gdt_info]
    ret

idt_install:
    lidt  [idt_info]
    ret

pmode_init:
    push  eax

    mov   eax, cr0
    or    eax, 1
    mov   cr0, eax

    pop   eax
    ret

gdt_info:
gdt_size  dw  0x18 - 1
gdt_ptr   dd  0x7E00 + gdt

gdt:
null_gdt  times 8 db 0
code_gdt  db 0xFF,0xFF,0x00,0x00,0x00,0x9A,0xCF,0x00
data_gdt  db 0xFF,0xFF,0x00,0x00,0x00,0x92,0xCF,0x00

idt_info:
idt_size  dw 0
idt_ptr   dd 0

%endif
