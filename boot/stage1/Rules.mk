SRC_STAGE1=$(wildcard $(SRCDIR_STAGE1)/*.s)
SRC_COMMON=$(wildcard $(SRCDIR_BOOT_COMMON)/*.s)
OBJ_STAGE1=$(patsubst %.s,%.o,$(patsubst $(SRCDIR_STAGE1)%,$(OBJDIR_STAGE1)%,$(SRC_STAGE1) ) )
OBJ_COMMON=$(patsubst %.s,%.o,$(patsubst $(SRCDIR_BOOT_COMMON)%,$(OBJDIR_STAGE1)%,$(SRC_COMMON) ) )

LD_STAGE1=-T $(SRCDIR_STAGE1)/linker.ld
AF_BOOT=-f elf32 -I $(SRCDIR_STAGE1)/

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_STAGE1)/%.s
	$(AS) $(AF_BOOT) -o $@ $^

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_BOOT_COMMON)/%.s
	$(AS) $(AF_BOOT) -o $@ $^

$(BINDIR)/boot.bin: $(OBJ_STAGE1) $(OBJ_COMMON)
	$(LD) $(LD_ALL) $(LD_STAGE1) -o $@ $^
