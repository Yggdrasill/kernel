OBJDIR:=build
BINDIR:=bin

AS=nasm
MKDIR=mkdir

INCLUDE_PATH=-I libk/ -I klibc/
CF_ALL=-m32 -ffreestanding -fno-pic -nodefaultlibs -fomit-frame-pointer \
	   -fno-exceptions -fno-asynchronous-unwind-tables -masm=intel -Wall -O0
LD_ALL=-m elf_i386
CFLAGS=-Wall -Wextra -pedantic

all: $(BINDIR) $(OBJDIR) $(BINDIR)/boot.bin

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
