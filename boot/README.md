Overview
--------

The overall architecture of the bootloader is as follows:

- **Stage 1**
    - Relocate boot code from 0x7C00 to 0x7E00.
    - Initialise segment registers.
    - Initialise 80x25 VGA text mode.
    - Read the disk and load stage 1.5 and 2 into memory.
- **Stage 1.5**
    - Set up all prerequisites for 32-bit protected mode.
    - Store relevant machine state required for later usage.
    - Implement mode switching, including a 32-bit/16-bit trampoline.
    - Enter 32-bit protected mode.
    - Read the stage 2 ELF from memory and jump to its entry point.
- **Stage 2**
    - Everything else e.g. memory management, I/O, filesystems, kernel loading.

Stage 1 and 1.5 are limited to the first 2048 bytes of disk space, where stage 2
will consume the remainder of the DOS compatibility (29.5KiB). The rationale is
that I would like to limit real-mode x86 assembly to as little as possible, and
write most of the bootloader in C.

Stage 2 can use stage 1.5's real-mode trampoline for performing BIOS calls, and
for that reason is linked against the binary objects produced by stage 1. The
way this is done is by leaving stage 1 code as a resident program in memory, but
it is not actually loaded into the program headers of the stage 2 ELF.

There is a linker script found in `./common/linker.lds.S` which is preprocessed
by the C preprocessors for specific stage linker configuration which should
serve as a good reference. This linker script does more than just linking the
binaries, as it also serves as a template for static memory allocation. The
linker exports a selection of symbols, which defines certain critical memory
regions. Examples of this are things like the E820 memory map, GDT, IDT, and
other machine-defined things such as common BIOS reserved areas.

Memory layout
-------------

This is based on the contents of `./common/linker.lds.S`, which remains the
authoritative source. The memory layout only describes memory <=1MiB, as any
other memory is more accurately described by the BIOS-provided memory map.

```
| Base address | End address | Description                  |
|--------------|-------------|------------------------------|
|       0x0000 |      0x0500 | BIOS interrupt vector table  |
|       0x0500 |      0x7C00 | Free memory                  |
|       0x7C00 |      0x7E00 | Stage 1, initial boot sector |
|       0x7E00 |      0x8000 | Boot relocation site         |
|       0x8000 |      0x8600 | Stage 1.5                    |
|       0x8600 |     0x10000 | Stage 2 (initially ELF load) |
|      0x10000 |     0x17800 | ELF relocation site          |
|      0x17800 |    Flexible | Statically allocated memory  |
|     Flexible |     0x70000 | Free memory                  |
|      0x70000 |     0x80000 | Execution stack              |
|      0x80000 |     0xA0000 | Extended BIOS data area      |
|      0xA0000 |     0xC0000 | VGA framebuffer              |
|      0xC0000 |    0x100000 | Upper memory reserved area   |
|--------------|-------------|------------------------------|
```

Trampoline
----------

This is specifically documentation with regards to the trampoline itself.
Further documentation of the trampoline can be found below, which I highly
recommend reading to understand its significant set of preconditions.

The real-mode trampoline is found within `./common/mode_switch.s`, and
implements a transition layer so that BIOS services can be called from stage 2,
which generally operates in 32-bit protected mode. This trampoline passes
arguments to the target function on the stack using an adapted version of the
SystemV 32-bit ABI. Its use of a union as a return type invokes Sret behaviour.

In implementing this adapted SystemV 32-bit ABI it saves callee-saved registers,
all relevant 32-bit protected mode machine state, and configures it according
to the real-mode machine state expected by the BIOS. When returning to 32-bit
protected mode it restores the protected-mode state and all callee-saved
registers. In order to make this work the trampoline performs some pretty
specific stack surgery.

The trampoline takes the callee function pointer as an argument, along with
whatever arguments the callee-function takes. Its C declaration is:

```C
extern union rmode_ret_t rmode_trampoline(void (*)(void), ...);
```

The trampoline is non-reentrant and supports data anywhere within the first 1MiB
of memory, but code is limited to the first 64KiB of memory. To understand its
stack behaviour the following diagrams may be helpful:

```
| Stack     | Description              |
|-----------|--------------------------|
| esp + 0   | Return pointer           |
| esp + 4   | Structure return pointer |
| esp + 8   | Callee function pointer  |
| esp + 12  | Callee argument n        |
| esp + 16  | Callee argument ...      |
|-----------|--------------------------|
```

The core of the trampoline is the following sequence of instructions:

```as
1  bits 16
2      pop  dword [resume]
3      pop  dword [sret_ptr]
4      pop  dword [callee]
5      push rmode_return
6      push word  [callee]
7      sti
8      ret
9  rmode_return:
10     cli
```

The lines 2-4 rewrite the stack, then lines 5-6 pushes 16-bit data:

```
| Stack   | Description         | => | Stack    | Description             |
|-------------------------------| => |-------------------------------------
| esp + 0 | Callee argument n   | => | esp + 0  | Callee function pointer |
| esp + 4 | Callee argument ... | => | esp + 2  | rmode_return pointer    |
|         |                     | => | esp + 4  | Callee argument n       |
|         |                     | => | esp + 8  | Callee argument ...     |
|---------|---------------------| => |----------|-------------------------|
```

There is more stack surgery performed within `rmode_trampoline`, but also
`pmode_exit` and `pmode_init`. This only documents the core sequence for the
trampoline itself.

MBR disk usage
--------------

There are certain limits that the size of our MBR bootloader can be. Stage 1
should consume as little space as possible, certainly no more than 31.5KiB, and
I can explain this number. It comes from what's called the DOS compatibility
region, which while originally for MS-DOS compatibility, is actually otherwise a
useful construct.

The disk addressing system on MBR systems is CHS, which means
cylinder-head-sector. The addressing mode here is related to disk geometry, and
since I cannot adequately represent this graphically in a text file, I would
suggest you read about it elsewhere. However, I can explain the concept of
tracks and how cylinders and heads fit into it. 

On hard drives a track is composed of 63 sectors, which are 512 bytes each. This
means that each track is 31.5KiB in size. These tracks are read by a particular
head, as an example reading from:

Cylinder = 1
Head = 1
Sector = 1

This reads the first 512 bytes of track 1 on head 1. The cylinder addresses the
across *all* heads, and the head address determines exactly which cylinder is
being read. The sector address determines which offset at that cylinder is read.

MS-DOS required that all partitions start on cylinder boundaries. Since the MBR
boot sector is the first 512 bytes of the disk, this means that MS-DOS could not
partition the first track. This leaves an empty space on the disk between the
MBR boot sector and the first possible partition for MS-DOS systems. This is
typically referred to as the DOS compatibility region, and became endemic due to
its usefulness. This is because 512 bytes of space is really not enough to do
much of anything.

Most bootloaders use this space for extra code, because it's practically
impossible to boot an entire system from 512 bytes alone. This is especially the
case considering many BIOSes assume that a BIOS Parameter Block exists and
clobber it. This leaves bootloaders with little choice but to reserve this
space, which narrows the room for code even more. Therefore most bootloaders
will use this 31.5KiB region for other boot code. Other software can also live
here, for instance rootkits may use this space.

Specific documentation
----------------------

Filenames should be searchable with a case-insensitive search.

-- common/print.s

    This file implements BIOS print interrupt calls, which is used for
    displaying messages on the screen during real mode execution. This is a
    practical consideration as it consumes far less space than writing to the
    VGA framebuffer.

-- common/mode\_switch.s

    This file implements CPU mode switching and all its prerequisites. That is,
    it installs a basic GDT and a null IDT, masking all interrupts in the
    process. What it does NOT do is enable the A20 gate, see stage1/a20.s for
    that.

    This file also implements a real mode trampoline, which enables me to pass
    arguments from 32-bit protected mode code to 16-bit real mode BIOS calls on
    the stack. Most bootloaders do this by passing arguments in the registers,
    but I think passing arguments on the stack is nicer. This does mean that
    there are some very real limitations however.

    IMPORTANT: READ ALL OF THIS. The stack ***MUST*** be located in low memory
    below 1MiB, as this is the only memory that real mode code can use. The
    stack base pointer CANNOT be on a 64K boundary. This is a very real design
    limitation that cannot be avoided while passing arguments on the stack. The
    trampoline is NOT RE-ENTRANT. The trampoline is NOT designed for nested
    calls (e.g. from interrupts). The trampoline does NOT reprogram the PICs,
    instead the IVT is aliased over the DOS-reserved region. There are a few
    additional limitations, documented under stage2/rmode.c in this README.

    So long as the preconditions for the trampoline to be used hold true, it
    will save all other machine state and restore it upon exit to the best of
    its ability. This does not save you from a broken machine state before
    entry. All callee-saved registers are handled within the trampoline.

    The other functions take care of actually putting the processor into 32-bit
    protected mode, or save and restore machine state inbetween transitions.
    A requirement for this is to provide the processor with a global descriptor
    table and an interrupt descriptor table. We provide the processor with a GDT
    that describes a flat memory structure, 4GB long. The GDT contains an eight
    byte long null descriptor, a code descriptor and a data descriptor. The base
    address is 0, and the limit is 0xFFFFF. The "granularity" bit is set, and so
    the limit is multiplied by 4096.

    0xFFFFF * 4096 = 4GiB

    A 16-bit real mode GDT entry is also provided for the purposes of
    re-entering real mode. This is used in the aforementioned trampoline to
    transition back to real mode, in order to execute BIOS calls from protected
    mode in C.

    We also provide the processor with a completely bogus IDT, and so interrupts
    ***MUST*** be disabled before we set the interrupt descriptor table, otherwise
    we risk triple-faulting the processor by sending it to a completely invalid
    interrupt handler. Interrupts should not be enabled from this point onwards,
    until we set up a proper IDT.

    The pmode_init function set's the protected mode bit in the CR0 register.
    After this point, we are officially in protected mode.

-- stage1/disk.s

    These implement wrapper functions for BIOS disk I/O. The reset function uses
    BIOS interrupt call int 0x13 ah=0x00 dl=drive to reset the disk drive, whether
    it be a floppy or a hard drive, which forces the drive to place its head at the
    first track, and makes the drive execute the next command as if it was in its
    initial state. This is done before a read operation just to be sure.

    The read function uses a relatively complex BIOS function call in which quite
    a few registers have to be set. It uses the int 0x13 ah=0x02 dl=drive BIOS
    interrupt call. For whatever reason, instead of the typical es:di combination
    for a memory pointer, this particular call requires es:bx to be the buffer
    pointer. The dh register tells the BIOS what head to read with, the al
    register is the number of sectors to read into memory. The cx register
    specifies where to read from, where ch is the track number and cl is the
    sector number (where for whatever reason, sectors count from 1).

-- stage1/a20.s

    This is the code to enable the A20 gate. The A20 gate is a consequence of x86
    backwards compatibility. Specifically, the original 8086 processor had two
    16-bit registers to address memory, and instead of just putting both linearly
    on the address bus, Intel decided that it'd be more desireable to shift the
    segment register 4 bits left so they could instead address 2^20 bytes. The
    offset register is *not* shifted, and with a segment register of 0xFFFF and
    and offset register of 0xFFFF, you can actually address 64kiB - 15B past 1MiB.

    (0xFFFF << 4) + 0xFFFF = 0x10FFEF

    The 80286 introduced a 24 bit address bus, and so to keep backwards
    compatibility with earlier processors, they introduced the A20 gate. This
    disabled the 21st bit of the memory bus, keeping the normal behaviour of the
    8086 processors. When addressing above the maximum addressable memory, the
    memory 20-bit address bus wraps around to 0. Since Intel are backwards
    compatibility fanatics they kept it in every following processor since then
    and it has become a permanent part of the x86 processor architecture. The
    permanency of this has only recently started to change.

    We can test the A20 gate by abusing the wraparound behavior. By testing what
    would be the same address after wraparound, we can compare the two words, and
    if they are the same, it's highly likely that that the A20 gate is not
    enabled. We are testing the terminating two bytes of the MBR header, 0xAA55.

    There are multiple ways to enable the A20 gate, and all implemented here need
    to be tried, because all of them may not work. They are tried in this order:

    - Ask the BIOS to do it. Ignore the response and test the address bus again,
    because the BIOS can lie about it.
    - Intel saw that there was a spare pin on the PS/2 keyboard controller, so
    they routed the A20 gate through there. Thus, this is one way to enable it.
    - Try an access of I/O port 0xEE. This does not work in QEMU, so it is
    untested, but it should work on a machine that supports it.
    - Fast A20 Enable is a feature of some motherboards, but is not guaranteed to
    work. Writing to this I/O port can have completely different effects or
    crash the machine, so this should be tried last.

    If none of the above work, we give up.

    Note that QEMU needs a SeaBIOS binary without the "enable A20" option in order
    to test this code, because if it is compiled with the "enable A20" option,
    naturally there would be no need to enable it, or even try. If you do build a
    SeaBIOS binary with the A20 gate disabled, you need to also run QEMU without
    the -enable-kvm switch, because as far as I can tell, -enable-kvm implicitly
    enables the A20 gate regardless of the SeaBIOS binary.

-- stage2/start.s

    Implements a short entry point that sets up a clean stack frame and calls
    the main() function in stage2/stage2.c. It also detects the CPU mode and
    refuses to enter stage 2 if in 16-bit mode.

-- stage2/stage2.c

    This is called by _start and is the true entry point of stage 2. main()
    should *never* return. There *is* a return pointer on the stack, but it
    simply halts the machine. If execution needs to stop the programmer should
    disable interrupts and use the hlt instruction.

    Note that all __asm__ should be followed by the volatile keyword to prevent
    the compiler from optimising it out of the code.

    System state at this point:

    - The A20 gate should be enabled before entering main()
    - The IDT should be installed and it should be zero length at address zero
    - The GDT should be installed and have a flat memory map, both the code and
    data descriptor
    - Protected mode should be enabled
    - The EFLAGS register should be cleared
    - The PIC should have all interrupts masked
    - All non-maskable interrupts should be disabled, and a valid shadow port
      0x70 value should be in memory.

    What should be done in stage 2:

    - The bootloader should set up a relatively usable IDT, although the kernel
      will very likely want to replace it later. This is needed to get device
      interrupts working, e.g. the keyboard.
    - The bootloader should unmask some of the interrupts on the 8259 PICs, for
      example the keyboard interrupt.
    - It should find the boot partition (partition table should be in memory at
      0x7DBE) and read the hard drive.
    - The partition should be an ext2 partition and the bootloader should read
      it, find a configuration file which will tell it what the kernel image is
      called and what it should put on the kernel command line.
    - It should be able to parse ELF executables, because that's likely what the
      kernel will use.

    Preferably, the code for the ext2 driver should be shared with the kernel,
    although of course, it will be statically linked.

-- stage2/rmode.c
    
    This file implements the actual bridging between C and the real mode
    trampoline found in common/mode\_switch.s. It uses inline assembly to push
    the arguments, target function pointer, and the return pointer with a call
    instruction. It implements an ABI between the real mode trampoline that
    allows one to transparently pass arguments to 16-bit real mode code, as if
    one were using the SystemV ABI normally. Neither the caller or callee can
    tell that the mode switch happened, nor that a transition from 32-bit to
    16-bit execution (and back) happened.

    The trampoline itself I think is fairly original, as it passes arguments on
    the stack, using the same stack between real and protected modes. Note that
    this comes with some important constraints that are unproblematic for the
    bootloader, but must be kept in mind:

    - The stack base must not be on a 64K segment boundary.
    - The stack must be located under 1MiB, as it is the maximum addressable
      memory of 16-bit x86.
    - Any pointers passed on the stack must be located under 1MiB.
    - Any executed code must be in cs segment 0x0000, limiting the target
      function address to the first 64K of memory.

    So long as these preconditions are true, the trampoline looks mostly like a
    regular SystemV ABI call. The only difference is that the target function
    must pop dword off the stack, and perform segment:offset address
    translation.

-- stage2/mmap.s

    The (rather long) function found here calls the BIOS to provide us with a
    map of usable, reserved and unknown memory. This actually uses the
    aforementioned real/protected mode trampoline. You can see the C code to
    bridge the SysV ABI function call to the trampoline in stage2/rmode.c.

    The specific BIOS function called is interrupt 0x15 eax=0xE820. Specific
    documentation for this particular BIOS call is widely available online.

    Short description: The BIOS provides us with a memory map in es:di, and the di
    register is not incremented. The BIOS keeps a "continuation value" in the ebx
    register, and the way we determine the end of the memory map is when ebx ==
    0. Unfortunately some BIOSes don't indicate this way and set the carry flag,
    so it must be tested.
