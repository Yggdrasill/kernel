%ifndef A20_S
%define A20_S

%include "bios.s"

; This is the code to enable the A20 gate. The A20 gate is a consequence of x86
; backwards compatibility. Specifically, the original 8086 processor had two
; 16-bit registers to address memory, and instead of just putting both linearly
; on the address bus, Intel decided that it'd be more desireable to shift the
; segment register 4 bits left so they could instead address 2^20 bytes. The
; offset register is *not* shifted, and with a segment register of 0xFFFF and
; and offset register of 0xFFFF, you can actually address 64kiB - 15B past 1MiB.
;
; (0xFFFF << 4) + 0xFFFF = 0x10FFEF
;
; The 8086 actually had a 24 bit address bus, and so when addressing memory
; limited to 20 bits, from the memory's point of view, the address bus would
; wrap around to 0 when passing the 1MiB address. Some programmers actually
; used this "feature", and since Intel are backwards compatibility fanatics
; they kept it in every following processor since then and it has become a
; permanent part of the x86 processor architecture.
;
; We can test the A20 gate by abusing the wraparound behavior. By testing what
; would be the same address after wraparound, we can compare the two words, and
; if they are the same, it's highly likely that that the A20 gate is not
; enabled. We are testing the terminating two bytes of the MBR header, 0xAA55.
;
; There are multiple ways to enable the A20 gate, and all implemented here need
; to be tried, because all of them may not work. They are tried in this order:
;
; - Ask the BIOS to do it. Ignore the response and test the address bus again,
;   because the BIOS can lie about it.
; - Intel saw that there was a spare pin on the PS/2 keyboard controller, so
;   they routed the A20 gate through there. Thus, this is one way to enable it.
; - Try an access of I/O port 0xEE. This does not work in QEMU, so it is
;   untested, but it should work on a machine that supports it.
; - Fast A20 Enable is a feature of some motherboards, but is not guaranteed to
;   work. Writing to this I/O port can have completely different effects or
;   crash the machine, so this should be tried last.
;
; If none of the above work, we give up.
;
; Note that QEMU needs a SeaBIOS binary without the "enable A20" option. If you
; do build a SeaBIOS binary with the A20 gate disabled, you need to also run
; QEMU without the -enable-kvm switch, because as far as I can tell, -enable-kvm
; implicitly enables the A20 gate regardless of the SeaBIOS binary.

a20_error:
    push  a20_err
    call  print
    cli
    hlt

a20_enabled:
    push  ax
    push  bx
    push  di
    push  si
    push  es
    push  ds

    xor   ax, ax
    mov   es, ax
    not   ax
    mov   ds, ax
    mov   di, 0x7DFE
    mov   si, 0x7E0E
    mov   ax, word [es:di]
    mov   bx, word [ds:si]

    pop   ds
    pop   es
    pop   si
    pop   di

    cmp   ax, bx
    je    a20_ne

    mov   [has_a20], byte 0x01
    push  a20_yes
    call  print
    add   sp, 2
    jmp   a20_end

a20_ne:
    push  a20_no
    call  print
    add   sp, 2

a20_end:
    pop   bx
    pop   ax
    ret

bios_a20:
    push  ax
    mov   ax, 0x2401
    int   0x15
    pop   ax
    ret

kbd8042_wait_cmd:
    in    al, 0x64
    test  al, 2
    jnz   kbd8042_wait_cmd
    ret

kbd8042_wait_data:
    in    al, 0x64
    test  al, 1
    jz    kbd8042_wait_data
    ret

kbd8042_a20:
    push  ax
    cli
    call  kbd8042_wait_cmd
    mov   al, 0xAD
    out   0x64, al

    call  kbd8042_wait_cmd
    mov   al, 0xD0
    out   0x64, al

    call  kbd8042_wait_data
    in    al, 0x60
    push  ax

    call  kbd8042_wait_cmd
    mov   al, 0xD1
    out   0x64, al

    call  kbd8042_wait_cmd
    pop   ax
    or    al, 2
    out   0x60, al

    call  kbd8042_wait_cmd
    mov   al, 0xAE
    out   0x64, al

    call  kbd8042_wait_cmd
    sti
    pop   ax
    ret

a20_ee:
    push  ax
    in    al, 0xEE
    pop   ax
    ret

a20_fast:
    push  ax
    in    al, 0x92
    or    al, 2
    out   0x92, al
    pop   ax
    ret

a20_init:
    call  a20_enabled
    cmp   [has_a20], byte 0x01
    je    done_a20

    call  bios_a20
    call  a20_enabled
    cmp   [has_a20], byte 0x01
    je    done_a20

    call  kbd8042_a20
    call  a20_enabled
    cmp   [has_a20], byte 0x01
    je    done_a20

    call  a20_ee
    call  a20_enabled
    cmp   [has_a20], byte 0x01
    je    done_a20

    cmp   [has_a20], byte 0x01
    jne   a20_error
done_a20:
    ret

a20_yes   db "A20 enabled",0x0D,0x0A,0
a20_no    db "Trying to enable A20",0x0D,0x0A,0
a20_err   db "Couldn't enable A20, giving up",0x0D,0x0A,0

has_a20   db 0

%endif
