OBJDIR:=build
BINDIR:=bin

AS=nasm
MKDIR=mkdir

INCLUDE_PATH=-I libk/ -I klibc/
CF_ALL=-m32 -ffreestanding -fno-pic -nodefaultlibs -masm=intel -Os
LD_ALL=-m elf_i386
CFLAGS=-Wall -Wextra -pedantic

all: $(BINDIR) $(OBJDIR) $(BINDIR)/boot.bin $(BINDIR)/stage2.bin

include boot/Rules.mk
include klibc/Rules.mk
include libk/Rules.mk

$(BINDIR):
	$(MKDIR) $(BINDIR)

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

clean:
	rm -rf bin
	rm -rf build
