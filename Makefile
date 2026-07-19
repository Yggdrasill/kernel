OBJDIR:=build
BINDIR:=bin

SRCDIR_STAGE1=boot/stage1
SRCDIR_STAGE2=boot/stage2
SRCDIR_BOOT_COMMON=boot/common
SRCDIR_LIBK=libk
SRCDIR_KLIBC=klibc

OBJDIR_STAGE1=$(OBJDIR)/stage1
OBJDIR_STAGE2=$(OBJDIR)/stage2
OBJDIR_LIBK=$(OBJDIR)/libk
OBJDIR_KLIBC=$(OBJDIR)/klibc
OBJDIR_GEN=$(OBJDIR)/generated

OBJDIRS=$(OBJDIR)        \
		$(OBJDIR_STAGE1) \
		$(OBJDIR_STAGE2) \
		$(OBJDIR_LIBK)   \
		$(OBJDIR_KLIBC)  \
		$(OBJDIR_GEN)

SRC_STAGE1=$(wildcard $(SRCDIR_STAGE1)/*.s)
SRC_COMMON=$(wildcard $(SRCDIR_BOOT_COMMON)/*.s)

OBJ_STAGE1=$(patsubst %.s,%.o,$(patsubst $(SRCDIR_STAGE1)%,$(OBJDIR_STAGE1)%,$(SRC_STAGE1)))
OBJ_COMMON=$(patsubst %.s,%.o,$(patsubst $(SRCDIR_BOOT_COMMON)%,$(OBJDIR_STAGE1)%,$(SRC_COMMON)))

DEPENDS=$(wildcard $(OBJDIR_STAGE1)/*.d) \
		$(wildcard $(OBJDIR_STAGE2)/*.d) \
		$(wildcard $(OBJDIR_LIBK)/*.d)   \
		$(wildcard $(OBJDIR_KLIBC)/*.d)  \
		$(wildcard $(OBJDIR_GEN)/*.d)

AS=nasm
MKDIR=mkdir -p

INCLUDE_PATH=-I ./ -I klibc/
CF_HOST=-m32 -std=c99
CF_ALL=$(CF_HOST) -ffreestanding -fno-pic -nodefaultlibs -fno-exceptions \
	   -fno-asynchronous-unwind-tables -masm=intel -Wall -Wpedantic -Os
CF_DEP=-MMD -MP -MF $(@:.o=.d) -MT $@
LD_ALL=-m elf_i386 -z noexecstack --nmagic
LD_BOOT=-L boot/common/
CFLAGS=-Wall -Wextra -pedantic

all: $(BINDIR)/boot.bin $(BINDIR)/stage2.elf

sinclude $(DEPENDS)
include $(SRCDIR_STAGE1)/Rules.mk
include $(SRCDIR_STAGE2)/Rules.mk
include $(SRCDIR_LIBK)/Rules.mk
include $(SRCDIR_KLIBC)/Rules.mk

$(BINDIR) $(OBJDIRS):
	$(MKDIR) $@

clean:
	rm -rf bin
	rm -rf build

debug: CFLAGS+=-g
debug: all
	objcopy --only-keep-debug bin/stage2.elf bin/stage2.debug
	strip --strip-debug --strip-unneeded bin/stage2.elf
	objcopy --add-gnu-debuglink=bin/stage2.debug bin/stage2.elf

image: all
image:
	dd if=/dev/zero of=image.img bs=512 count=2880
	dd if=bin/boot.bin of=image.img conv=notrunc bs=512 count=4
	dd if=bin/stage2.elf of=image.img conv=notrunc bs=512 seek=4
