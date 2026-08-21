Description
-----------

This directory implements functionality shared between stage 1, 1.5 and stage 2.
These files are loaded in stage 1 and 1.5, but simultaneously linked with NOLOAD
to stage 2, with a NONE program header. The functionality present here is:

- All mode switching.
- All CHS disk reading code.
- The BIOS string output functions.

The latter is not used really by stage 2, as stage 2 can write directly to the
framebuffer. However, it is used by both stage 1 and 1.5, and so also resides in
here.

Warning
-------

There be dragons. The code found herein is very aggressively space optimised, at
times very obscure, and at times written with exact instruction encoding in
mind. This all comes from the fact that stage 1 is limited to 512 bytes, and
stage 1.5 to 1536 bytes. **DO NOT** modify this code carelessly. Treat it with
the thought, love, and care that it demands. 

I have done my very best to write explanatory comments within these files to
explain what's going on. However, you should expect to find all the dirty tricks
within these files. This includes:

- Mode-independent instruction decoding.
- Mode-dependent instruction decoding.
- Instruction overlapping.
- Self-modifying code.
- Manually reinterpreting stack pointers.
- Rewriting the stack.
- Stack surgery of various kinds.
- Manipulating stack frames directly.
- Legitimate use of light return-oriented programming.

With all that said, in return for this, the project gets several nice features.
Within just 2KiB of space it implements the bare minimum to get the following:

1. Enabling the A20 line and switching to protected mode.
2. A reusable CHS disk reader that is callable both from 16-bit and 32-bit
   protected mode.
3. A reusable real mode trampoline which stage 1.5 uses to read the disk, which 
   also gets linked into stage 2 as NOBITS.
4. A minimal ELF loader capable of loading stage 2 as an ELF from disk, possibly
   with multiple program headers.

What follows is documentation of all this.

ABI
---

The ABI is derived from the SystemV 32-bit ABI, which acts like a bit of a
restricted subset of it. The preconditions are documented in the next section,
but I will cover the important overall ABI here. The purpose is to call a 16-bit
real mode function from 32-bit protected mode, as if it was a regular function.
This is done by passing a function pointer and variadic arguments on the stack.
As per the SystemV ABI all passed arguments are 32-bit arguments, i.e. 4 bytes
in size.

The valid callee functions in this ABI are prefixed by two underscores, for
example `__bios_mmap`. Any other assembly function will not adhere to this ABI,
and are not valid callees. Valid targets must reside in the first 64KiB of
memory, and any data passed naturally must be within the first 1MiB of memory
due to 16-bit real mode addressing limitations.

Valid callees with the double-underscore prefix are 16-bit real mode functions
that accept 4-byte arguments using the operand override prefix. The argument
order is very simple, and a call might look like:

```C
/* ./boot/stage2/rmode.c */
int32_t bios_mmap(struct e820_info *mmap)
{
    union rmode_ret_t rv;
    rv = rmode_trampoline((void (*)(void))__bios_mmap, mmap);
    /* snip */
    return rv.i32;
}
```

It is generally recommended that typed wrappers surround the `rmode_trampoline`
calls, and for that reason the declaration of this `extern` function is found
only in `./boot/stage2/rmode.c`.

Trampoline
----------

The real mode trampoline is found within `mode_switch.s`, and implements a
transition layer so that BIOS services can be called from stage 2, which
generally operates in 32-bit protected mode. This trampoline passes arguments on
the stack with the ABI that I described above. Its use of a union as a return
type invokes Sret behaviour.

In implementing this adapted SystemV 32-bit ABI it saves callee-saved registers,
all relevant 32-bit protected mode machine state, and configures it according
to the real mode machine state expected by the BIOS. When returning to 32-bit
protected mode it restores the protected-mode state and all callee-saved
registers. In order to make this work the trampoline performs some pretty
specific stack surgery.

The trampoline takes the callee function pointer as an argument, along with
whatever arguments the callee-function takes. Its C declaration is:

```C
extern union rmode_ret_t rmode_trampoline(void (*)(void), ...);
```

The trampoline is non-reentrant and supports data anywhere within the first 1MiB
of memory, but code is limited to the first 64KiB of memory. It also supports
paging, though strongly preferred with identity mapping of the entire low memory
region <=1MiB. As implied by its C declaration above it accepts a variadic
number of arguments, limited only by the size of the stack. To understand its
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

The lines 2-4 rewrite the stack, then lines 5-6 push 16-bit return pointers:

```
| Stack   | Description         | => | Stack    | Description             |
|-------------------------------| => |-------------------------------------
| esp + 0 | Callee argument n   | => | esp + 0  | Callee function pointer |
| esp + 4 | Callee argument ... | => | esp + 2  | rmode_return pointer    |
|         |                     | => | esp + 4  | Callee argument n       |
|         |                     | => | esp + 8  | Callee argument ...     |
|---------|---------------------| => |----------|-------------------------|
```

There is more stack surgery performed within `rmode_trampoline`, but there's
also specific return pointer behaviour to `pmode_init` and `pmode_exit`. The
purpose of this is to document the core instruction sequence itself, and also
usage of the trampoline more generally. The trampoline does have some very
strict preconditions for use. I will outline them first, and then answer with a
rationale for why they are acceptable:

1. Any real mode code called must reside in the first 64KiB of memory.
2. If paging is enabled, then any executable code  in real mode must be identity
   mapped. Particularly the mode switching itself must be identity mapped, as
   documented by the Intel programmer's manual (12-14 Vol. 3A).
3. The stack and any pointers passed must be an address below 1MiB, as it is the
   limit of the 16-bit real mode segment:offset type addresses.
4. The stack must be identity mapped, any buffers, pointers etc. passed to
   real-mode code are identity mapped.
5. The address space as a whole is flat and all segments selectors have base
   zero.
6. The stack pointer is not aligned on a 64K boundary, as it breaks the stack
   reinterpretation into 16-bit segment:offset addresses.
7. Either one of the following: The PICs have not been remapped from the
   traditional real mode mappings of 0x08-0x0F/0x70-0x77. Alternatively, the IVT
   has been aliased with the real mode vectors for IRQs 0-15 copied into the
   appropriate configured vector offsets. This should be within the IVT range
   0x20-0x3F.
8. It is not reentrant and cannot be nested, and should never be called from
   exception or interrupt handlers.

You might ask: Why use such a trampoline? These preconditions seem quite brutal.
Well, the answer is that a significant portion of the mode-switching code is
required simply to get into 32-bit protected mode, and this solution ends up
being very space efficient, and also very flexible provided its preconditions
hold true. Additionally, the arguments for why I accept all these preconditions
are as follows:

1. The bootloader is limited to the 31.5KiB DOS compatibility region anyway.
   This will never become a problem, and if it does, it is trivial to link
   real mode code into the first 64KiB of memory.
2. Not only does Intel document this themselves, but not requiring this to be
   the case would end up with a whole lot of extra code. The bootloader itself
   is likely to be completely identity mapped anyway, so it doesn't matter.
3. This is a natural limitation for any real mode code, and to get around it one
   would have to do extensive copying of arguments and data. It is simpler to
   just allocate space for this data and the stack below 1MiB to begin with.
4. This is more or less just an extension of point #2. It is a natural
   consequence of the operation itself, not actually a problem.
5. Not only does the SystemV i386 ABI kind of require a flat memory space, but
   so does any modern and reasonable compiler. Even the modern compilers that do
   not require this are happy to compile code for such an environment.
6. This is easy to solve by simply putting the stack at an address just below a
   64K boundary. As an example, the linker script in this directory puts it
   at 0x7FFF0, and gives it 64K - 16 bytes of space. This leaves plenty of space
   for the stack to not wrap on the lower end of the 64K window as well.
7. See libk/irq.c for details. Any IBM-compatible machine must leave this range
   reserved for MS-DOS, which since this is not, we should be able to use. What
   I will say is that the alternative is to reinitialise the PICs with ICWs
   every time `rmode_trampoline` is called, which is equally nasty. Doing that
   loses PIC 8259A state, and can end up with missed interrupts, which my chosen
   solution ends up avoiding entirely. As a bit of evidence, it works fine on
   the two physical machines I have tested it on so far.
8. The bootloader is single-threaded and non-concurrent anyway. I can see no
   legitimate reason why a 32-bit exception/interrupt handler should drop down
   to 16-bit real mode. The BIOS should be irrelevant for any real kernel
   anyway, so this is honestly totally fine.

Mode switching
--------------

Due to the space constraints and the nature of the problem solved, the code
found in these files can be quite confusing. As an example, consider this
call to `mode_switch.s:pmode_init`:

```as
bits 16
    call  0x0000:pmode_init

pmode_init:
1     and   esp, 0xFFFF
2     push  eax

[...]

bits 32
19    pop   eax
20    ret
```

The function was called as a far call from 16-bit mode, meaning that a pair of
16-bit values was pushed on the stack. This totals to 4 bytes, which then gets
reinterpreted by the 32-bit ret instruction as a regular return pointer. This
function also rewrites the stack pointers from 16-bit segment:offset pointers to
32-bit stack pointers. The reverse behaviour in all respects takes place in
`pmode_exit`, where a 16-bit retf instruction interprets the 32-bit return
pointer as a segment:offset pair, with code segment zero.

There's a much more confusing piece of code found within `mode_switch.s`, but it
will require some context. During mode transition it is quite important to do
the following things:

1. Clear the interrupt flag.
2. Mask all interrupts.
3. Disable non-maskable interrupts by a write to port 0x70.

These are all state that needs to be appropriately restored after mode
transition, but unfortunately one cannot necessarily read port 0x70 to know NMI
state. The solution to this is to track a shadow value, which in this case is
stored in a byte variable named `shadow_p70`. Due to the amount of functionality
packed into stage 1/1.5, and resulting space constraints, I have had to devise a
way to read this variable in both 16-bit and 32-bit modes, without duplicating
the code. The problem is that `mov` operand sizes are different between 16-bit
and 32-bit modes, and the operand override prefix changes the size in both
modes. The solution is:

```as
bits 16
1  get_shadow_p70:
2      xor   cx, cx
3      push  strict word shadow_p70
4      dec   cl
5      pop   cx
6      jns   short fix_shadow_p70+1
7  fix_shadow_p70:
8      movzx ecx, cx
9      jns   short load_shadow_p70+1
10 load_shadow_p70:
11     mov   al, [ecx]
12     sahf
13     jnc   short ms_nmi_disable_ret
14     jmp   short restore_p70_ret
15
16 ms_nmi_disable:
17     push  ax
18     clc
19     lahf
20     jmp   short get_shadow_p70
21 ms_nmi_disable_ret:
22     or    al, 0x80
23     out   0x70, al
24     pop   ax
25     ret
26
27 restore_p70:
28     push  ax
29     stc
30     lahf
31     jmp   short get_shadow_p70
32 restore_p70_ret:
33     out   0x70, al
34     pop   ax
35     ret
```

First the entry points are `ms_nmi_disable` and `restore_p70`. The former clears
the carry flag, and the latter sets it, then both of them store it into `ah`
with the `lahf` instruction. This is important, because the return point needs
to be tracked, but since the call instruction operand size also varies between
16-bit and 32-bit modes it cannot be used. You might see the pattern now, where
both `ms_nmi_disable` and `restore_p70` use only instructions with the same
operand sizes in both modes. In any case, both of them jump to `get_shadow_p70`.

There are four different ways to enter `get_shadow_p70`. Both `ms_nmi_disable`
and `restore_p70` are called in both 16-bit and 32-bit modes from
`rmode_trampoline`, which means that `get_shadow_p70` needs to decode the
address and load al with the same result between both modes. The `push`
instruction on line 2 uses the `strict word` keywords to force nasm to never
emit an operand override prefix. The `movzx` on line 8 and `mov` on line 11 emit
the operand override and address override prefix byte respectively.

Now, the code executes as you would expect under 16-bit mode, because the `dec`
instruction on line 4 executes and sets the sign bit none of the `jns` branches
are taken. That is **not** the case under 32-bit mode, however. and the trick
lies in line 3. The `push strict word shadow_p70`, as I explained earlier, does
not emit the operand override prefix, hence the two bytes of `dec cl` are
consumed. Appropriately, since the sign flag was cleared by `xor` and the `dec`
instruction was never executed, the sign flag is **not** set. The `jns` branches
are now taken, at a 1-byte offset into `movzx`, which strips them of the
operand/address override prefixes, and the CPU interprets them as it should.

In 16-bit mode the instructions of lines 2-5 are decoded as:

```
0x31C9 ; xor cx, cx
0x68iw ; push shadow_p70, .e.g. iw is 2 bytes = &shadow_p70
0xFEC9 ; dec cl
0x59   ; pop cx
```

In 32-bit mode the same instructions are decoded as:

```
0x31C9     ; xor ecx, ecx
0x68iwFEC9 ; push ((0xFEC9 << 16) | &shadow_p70)
0x59       ; pop ecx
```

This setup now allows the rest of the code to use the sign flag as a mode
discriminant. Since the operand/address override of `movzx` and `mov` on lines 8
and 11 respectively would cause 32-bit execution to interpret the operand size
as 16-bit it must be stripped, which `jns` does by jumping into the instruction
by a byte. Since `ah` was not touched during the entire operation, the carry
flag is then restored by `sahf`, and the function returns to the appropriate
place.

Important to note is that the `movzx` instruction actually serves a purpose in
both modes, as it cleans up the consumed `dec cl` instruction in 32-bit mode,
and otherwise cleans up the high bits of `ecx` in 16-bit mode since the `xor`
never did.

Hooks
-----

This is mostly related to `disk.s`. The intention is that the CHS `int 0x13`
disk code which stage 1 already makes use of should be reused as a fallback, in
the case that BIOS EDD isn't available. It is also used for loading the ELF of
stage 2 from stage 1.5. This involves hooking the functions and disrupting their
normal control flow. The original code does very little in terms of error
reporting, and so it clobbers the ah status code. Moreover, under error
conditions it makes a fatal call to `__bios_error` which hangs the machine.
Since we are now in protected mode with vastly more space available, we'd like
to handle these things a bit more gracefully. We want to keep the ah status code
intact and deal with errors properly.

This leads to the initial hook setup, placed in the common `int 0x13` interface
of all `disk.s` functions:

```as
bits 16
1  int13:
2      push  es
3      push  ds
4      int   0x13
5      pop   ds
6      pop   es
7  int13_hook:
8      jmp   strict near int13_ret
9  int13_ret:
10     ret
```

The `jmp` instruction is forced to be encoded as the `0xE9 cw` instruction.
Initially it jumps +0 bytes, but has now opened a window to hook the exit point
of this function. This is done by the following code, shared in common with all
the valid trampoline targets:

```as
bits 16
1     mov   ax, hook_return
2     call  hook_install
[...]
3 hook_install:
4     mov   si, int13_hook
5     sub   ax, int13_ret
6     mov   [ds:si + 1], ax
7     ret
```

This poses a bit of a problem, however. Functions such as `__chs_geometry` call
the target function normally, which in turn calls `int13`. When the hook is is
jumped to the two return pointers are still on the stack, but this is not
desired. The target of `__chs_geometry` is `disk_geometry`, which does not
iterate in a loop. These return pointers must be removed without altering the
carry flag, so arithmetic is generally not suitable.

```as
; __chs_geometry
1     push  di
2     call  disk_geometry
3 geometry_hook:
4     ; Get rid of two return pointers
5     ; without touching flags.
6     pop   esi
7     pop   di
8     jc    hook_exit
9     call  geometry_done
```

The `pop esi` instruction on line 6 is responsible for removing them, as it pops
4 bytes off the stack, i.e. 2x 2-byte return pointers. On error the carry flag
is set, and the function returns with the `ah` status code in eax. Otherwise, in
order to save space the code then returns control briefly to `disk_geometry` at
a different entry point. This extracts the CHS values from the registers and
then returns, at which point `__chs_geometry` writes the information into a
`struct disk_info` data structure.

This all gets a little bit more complicated with the `read` function found
within `disk.s`, since it iterates one sector at a time. It also has a second
place where it can call `__bios_error`, which means a second hook must be
installed. The installed hooks for this function are:

```as
;  __chs_read
1      mov   ax, read_hook
2      call  hook_install
3      mov   ax, read_error_hook
4      mov   si, read_error
5      sub   ax, read_done
6      mov   [si + 1], ax
[...]
; hooks
64 read_hook:
65     pop   bp
66     movzx ecx, ah
67     lea   ecx, [ecx - 0x11]
68     jcxz  hook_ecc
69     jmp   hook_save
70 hook_ecc:
71     xor   ah, ah
72 hook_save:
73     mov   [status], ah
74     popa
75 jmp_success:
76     jnc   read_success
77     jmp   read_hook_ret
78 read_error_hook:
79     mov   [status], byte 4
80 read_hook_ret:
81     ret
```

The intention here is to return to the appropriate function. Under error
conditions control is returned to `__chs_read`, whereas if the read was
successful then control is returned to `read`, which is also the case for ECC
corrected reads. First focus your attention on `read_hook`, as it is the hook
which is jumped to by `int13`.

First, the `pop` instruction removes the return pointer of `int13` from the
stack. The `lea` instruction is useful here because it can perform arithmetic
without messing up the carry flag. The `ah` status code is zero-extended into
`ecx`, which performs the `lea` to subtract the ECC corrected error code from
itself without altering the flags. The `jcxz` instruction is then used to branch
if it evaluates to zero, so what this actually implements is comparison without
changing the flags. If this was an ECC recovered error, `xor` then both clears
the carry flag and the status code to 0 (success).

Next, the `read` function uses a pusha instruction before the call to `int13`,
which leaves a large stack frame that needs to be dealt with, but not before we
save the status code of `ah`. Besides the conditionally executed `xor` earlier,
the carry flag has not been altered, so if it is not set or ECC was corrected,
the read is considered successful and control is returned to `read`. Besides ECC
corrected, any other disk errors returns control to `__chs_read`, which
ultimately returns to the caller.

Now it is time to discuss the second hook, which is `read_error_hook`. The hook
is installed towards the end of the `read` function:

```as
; read
1 read_next_cylinder:
2     ; unpack, calculate, repack CHS addresses
3     cmp   ax, [disk_cylinders]
4     jbe   read_sector
5 read_error:
6     jmp   strict near read_e
7 read_done:
8     ret
```

If for whatever reason we have exceeded the maximum cylinders on the disk, this
would normally call `__bios_error`. This is unacceptable, as it hangs the
machine as if it was a panic call. For that reason it is necessary to also hook
this `jmp`, which ends up in `read_error_hook` as shown above. Since this is
within the `read` function itself and with no `pusha` stack frame, the return
pointer on the stack is pointing back to `__chs_read` which we entered from. In
this case, set the error code to 4 (refer to RBIL int 0x13 ah=0x01), then return
and exit.

Explained with a poorly drawn diagram that was an absolute pain to do:

```
            rmode_trampoline
                    |
               __chs_read
                    |
                  read
                    |
                  int13
                    |
                 read_hook
                   /  \
                  /    \
                 /      \
                /        \
               /          \
              /            \
             /              \
       read_success      __chs_read
         /      \             |
        /        \            |
  read_sector read_error_hook |
       |           \          |
       |            \         |
 loop until done     __chs_read
       |             /
  __chs_read        /
       |           /
     rmode_trampoline
```
