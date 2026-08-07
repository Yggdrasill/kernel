Description
-----------

A work-in-progress bootloader and kernel written in C and a fair bit of x86
assembly. The goal is a bootloader and microkernel that runs on real hardware.
These are both substantial projects in their own right, as I would like both
pieces of the project to stand on their own merits. The kernel has yet to be
started.

The project currently runs on physical x86 hardware on all the machines which I
have tested. That is, I have tested it on a Thinkpad X220 laptop, and a desktop
computer with a Ryzen 5800X3D on a Gigabyte X370 motherboard. I would expect
that the project runs on most x86 IBM-compatible machines, but I cannot promise
that every machine will run it, especially not with highly malbehaving BIOSes.

Please see `./boot/README` for technical documentation about the boot process
and overall architecture of the bootloader. Such documentation will follow for
the kernel when the shape of that problem becomes more apparent to me.

**"Why roll your own bootloader?"**

This is a good question, as bootloaders are themselves quite complex to
implement. There are good bootloaders out there like GRUB or Limine that would
have brought me to a kernel much faster, and would have enabled me to just get
started with the kernel immediately. Many hobby OS projects do exactly this, for
very justifiable reasons. Bootloaders are difficult to write and take a lot of
time to do well.

I have committed to this project for both fun and learning. The boot process on
x86 is fairly complicated, sometimes arcane, and it seemed like an interesting
challenge. It has indeed taught me a lot, and the constraints which it has put
me under has resulted in some pretty interesting problems and solutions. As an
example I would suggest to have a look at `./boot/common/mode_switch.s`.

Roadmap
-------

The bootloader is not complete, but this will hopefully shine some light on the
progress so far, and what remains to be done:

- Initial boot stage
- Load stage 1.5, stage 2
- ELF load and relocate stage 2
- Real mode trampoline (32-bit to 16-bit BIOS calls)
- E820 memory map discovery and sanitisation
- Dynamic memory management (physical, heap)

To be implemented:

- BIOS EDD discovery and disk reading
- Read-only ext2 implementation
- General ELF parsing and loading
- Configuration file parsing
- Boot protocol definition and implementation
- Load a kernel

License
-------

GPLv2 since commit `232bfe954369ce54fd85f87c7009389ab1db01cd`. See the COPYING
file for details. This license subject to change, likely to GPLv3 or similar.

Prior to commit `9c470f1168b490c6fbba376f341152d8ac393cda` the project is
GPLv2-only, as the E820 sanitiser algorithm could be considered a derivative
work. This derivative algorithm has been replaced by my own implementation.

Requirements
------------

More or less any Linux machine or WSL configuration should be able to build the
project. Other systems e.g. various BSDs should be able to build the project
provided the dependencies are satisfied. The build dependencies are:

- GNU Make
- GNU/LLVM ld
- GCC/clang
- POSIX shell
- binutils
- nasm
- awk
- xxd

**Rationale:**

Unfortunately Assembly languages are not very portable between different
assemblers, as any assembler can have their own dialect. This uses the Netwide
Assembler, or nasm, and will likely use this assembler forever.

While the project intends to be reasonably compiler/toolchain-portable it is
currently not fully. It requires a linker that understands GNU ld's
linkerscripts, and the build system also depends on GNU Make. However, it should
be compiler-portable otherwise, as the project does not use
`__attribute__((section))` or `__attribute__((packed))` like many OS projects
do. All the structs that operate with CPU data structures, such as the GDT and
IDT, are manually aligned and packed.

The only requirements of the compiler is that it has some reasonable
implementation-defined behaviours (per the C99 standard). As an example, the
project's memory allocators rely on converting pointers to integer values
(`uintptr_t`) in order to perform arithmetic.

Building
--------

To build the project itself, simply:

`make`

You will be left with bin/boot.bin and bin/stage2.elf. Alternatively, you can
build a basic floppy disk image:

`make image`

This will create a file called image.img, which can be run by any virtual
machine software.

Manual image creation
---------------------

**USB**

If you have an UEFI system it will not be bootable until you enable CSM mode, as
UEFI compatibility has yet to be implemented. The partition table on the target
USB device should be of type `msdos`/`MBR`, which a partitioning tool like
(g)parted should be able to do for you. Be sure to set the `boot` flag as well,
otherwise many BIOSes are likely to ignore it as a valid bootable device.

To create a bootable USB image you can use the following commands. Just be sure
to substitute the target `/dev/sdX` with your actual USB device.

**WARNING:** There is a reason why `dd` is often referred to as "destroy disk!"
Be absolutely **certain** that you are targeting the correct block device. I am
not responsible for any data loss incurred.

```
# dd if=bin/boot.bin bs=446 count=1 conv=notrunc of=/dev/sdX
# dd if=bin/boot.bin bs=512 skip=1 seek=1 count=3 conv=notrunc of=/dev/sdX
# dd if=bin/stage2.elf bs=512 seek=4 conv=notrunc of=/dev/sdX
```

**QEMU/Bochs**

This describes the manual creation process for a basic image. The image can be
created by running the following commands:

```
$ dd if=/dev/zero bs=512 count=2880 of=image.img
$ dd if=bin/boot.bin bs=512 count=4 conv=notrunc of=image.img
$ dd if=bin/stage2.elf bs=512 seek=4 conv=notrunc of=image.img
```

Running
-------

To run the project in a QEMU VM, using the basic image created by make:

```
qemu-system-i386 image.img
qemu-system-i386 -enable-kvm image.img
```

Your particular Linux distribution may call the executable for an i386 QEMU
environment something else. Its name on Debian GNU/Linux is qemu-system-i386.

If you instead prefer to use Bochs, which can be useful at times, this is a
basic `.bochsrc` file appropriate for the basic image created by Make:

```
floppya: 1_44="image.img", status=inserted
boot: floppy
vgaromimage: file="/usr/share/seabios/vgabios.bin"
display_library: sdl2, options="gui_debug"
```

This may not be completely appropriate for your particular Linux distribution.
For instance, the VGA ROM image may be located somewhere else, or have a
different name.

Binaries
--------

bin/boot.bin: The MBR boot image that occupies a maximum of 2048 bytes of space
on the disk. This is the bootloader's first stage, containing the immediate MBR
sector in the first 512 bytes and stage 1.5 which are an additional 1536 bytes
of code/data. This is a raw binary file of mixed 16-bit and 32-bit x86 machine
code. It contains minimal amounts of code to:

    - read the disk
    - print out error messages
    - set up a basic GDT and null IDT, mask all interrupts
    - enter protected mode
    - do a very basic ELF load of stage 2
    - trampoline switch between real/protected modes

bin/stage2.elf: Stage 2 of the bootloader, in an ELF executable file. This is
limited to ~30KiB as it must fit within the remaining space in the DOS
compatibility region. This will use the trampoline described in stage 1 to
perform real-mode BIOS calls for detecting hardware and configuring the system.

While UEFI compatibility is currently unimplemented it will share much code with
stage 2, besides the parts which naturally must differ. This is because the UEFI
loader needs to query the UEFI image for information, whereas the MBR entry
point needs to make real-mode BIOS calls. UEFI support is a long way off and not
likely to be implemented any time soon.

This is currently not complete, but the general plan is as follows:

    - Perform real-mode calls for BIOS functions, for example the E820 memory
      map, gather hardware information, configure VGA modes etc.
    - Manage memory, allocate and deallocate buffers as needed.
    - Beyond the very basics like a PS/2 keyboard driver, the bootloader will
      not have any drivers of its own. It will rely on BIOS/UEFI for its needs.
    - The drivers it does have will be shared by the kernel, though most likely
      only a minimal subset of them. An example of such a driver is ext2.
    - Load the kernel ELF image into memory and transfer control, passing all
      required information on the stack.

Notes
-----

On modern systems with UEFI and LBA disk addresses (flat address space), it is
typically the case that partitions are aligned alongside 1 MiB boundaries. The
reasons for this are actually quite varied, but almost all partitioning tools
leave a gap of this size.

This README will be extended as the project grows in size, as it presently only
consists of the bootloader. A README documenting the bootloader's architecture,
design, and various x86 things can be found in `./boot/`.
