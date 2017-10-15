CFLAGS=-m32 -ffreestanding -fno-pic -nodefaultlibs -masm=intel -Os -Wall -Wextra -pedantic
LDFLAGS=-T linker.ld -m elf_i386

all: boot.bin stage2.bin

boot.bin:
	nasm -f bin -o boot.bin boot.s

stage2.bin:
	nasm -f elf32 -o asm_exception.o exception.s
	nasm -f elf32 -o asm_irq.o irq.s
	$(CC) $(CFLAGS) -c -o stage2.o stage2.c
	$(CC) $(CFLAGS) -c -o string.o string.c
	$(CC) $(CFLAGS) -c -o idt.o idt.c
	$(CC) $(CFLAGS) -c -o io.o io.c
	$(CC) $(CFLAGS) -c -o irq.o irq.c
	$(CC) $(CFLAGS) -c -o interrupt.o interrupt.c
	$(LD) $(LDFLAGS) -o stage2.bin stage2.o string.o idt.o io.o irq.o asm_exception.o asm_irq.o interrupt.o

clean:
	rm boot.bin stage2.bin
	rm *.o
