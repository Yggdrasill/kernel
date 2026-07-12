bits 32
section .text

global nmi_enable
global nmi_disable

extern shadow_p70

nmi_enable:
	push ebp
	mov  ebp, esp
	push eax
	mov  al, [shadow_p70]
	and  al, 0x7F
	out  0x70, al
	mov  [shadow_p70], al
	pop  eax
	pop  ebp
	ret

nmi_disable:
	push ebp
	mov  ebp, esp
	push eax
	mov  al, [shadow_p70]
	or   al, 0x80
	out  0x70, al
	mov  [shadow_p70], al
	pop  eax
	pop  ebp
	ret
