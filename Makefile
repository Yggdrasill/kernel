OBJDIR:=build
BINDIR:=bin

SRCDIR_STAGE1=boot/stage1
SRCDIR_STAGE2=boot/stage2
SRCDIR_BOOT_COMMON=boot/common
SRCDIR_KLIBC=klibc
SRCDIR_LIBK=libk

OBJDIR_STAGE1=$(OBJDIR)/stage1
OBJDIR_STAGE2=$(OBJDIR)/stage2
OBJDIR_KLIBC=$(OBJDIR)/klibc
OBJDIR_LIBK=$(OBJDIR)/libk

SRC_STAGE1=$(wildcard $(SRCDIR_STAGE1)/*.s)
SRC_COMMON=$(wildcard $(SRCDIR_BOOT_COMMON)/*.s)

OBJ_STAGE1=$(patsubst %.s,%.o,$(patsubst $(SRCDIR_STAGE1)%,$(OBJDIR_STAGE1)%,$(SRC_STAGE1) ) )
OBJ_COMMON=$(patsubst %.s,%.o,$(patsubst $(SRCDIR_BOOT_COMMON)%,$(OBJDIR_STAGE1)%,$(SRC_COMMON) ) )

AS=nasm
MKDIR=mkdir -p

INCLUDE_PATH=-I libk/ -I klibc/
CF_ALL=-m32 -ffreestanding -fno-pic -nodefaultlibs -fno-exceptions \
	   -fno-asynchronous-unwind-tables -masm=intel -Wall -O0
LD_ALL=-m elf_i386 -z noexecstack --nmagic
CFLAGS=-Wall -Wextra -pedantic

all: $(BINDIR) $(OBJDIR) $(BINDIR)/boot.bin $(BINDIR)/stage2.elf

include $(SRCDIR_STAGE1)/Rules.mk
include $(SRCDIR_STAGE2)/Rules.mk
include $(SRCDIR_KLIBC)/Rules.mk
include $(SRCDIR_LIBK)/Rules.mk

$(BINDIR):
	$(MKDIR) $(BINDIR)

$(OBJDIR):
	$(MKDIR) $(OBJDIR)
	$(MKDIR) $(OBJDIR_STAGE1)
	$(MKDIR) $(OBJDIR_STAGE2)
	$(MKDIR) $(OBJDIR_KLIBC)
	$(MKDIR) $(OBJDIR_LIBK)

clean:
	rm -rf bin
	rm -rf build

debug: CFLAGS+=-g
debug: all
