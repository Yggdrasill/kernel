CFLAGS=-m32 -ffreestanding -fno-pic -nodefaultlibs -masm=intel
LDFLAGS=-T linker.ld -m elf_i386

all: boot.bin stage2.bin

boot.bin:
	nasm -f bin -o boot.bin boot.s

stage2.bin:
	$(CC) $(CFLAGS) -c -o stage2.o stage2.c
	$(CC) $(CFLAGS) -c -o string.o string.c
	$(CC) $(CFLAGS) -c -o idt.o idt.c
	$(LD) $(LDFLAGS) -o stage2.bin stage2.o string.o idt.o

clean:
	rm boot.bin stage2.bin
	rm *.o
