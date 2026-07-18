LD_STAGE1=-T $(SRCDIR_STAGE1)/linker.ld -T $(SRCDIR_BOOT_COMMON)/s1_bootdefs.lds
AF_BOOT=-f elf32 -I $(SRCDIR_STAGE1)/

$(SRCDIR_BOOT_COMMON)/s1_bootdefs.lds: $(SRCDIR_BOOT_COMMON)/bootdefs.lds.S
	$(CC) $(INCLUDE_PATH) -DLD_BOOT_STAGE1 -E -P -x c -o $@ $^

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_STAGE1)/%.s
	$(AS) $(AF_BOOT) -o $@ $^

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_BOOT_COMMON)/%.s
	$(AS) $(AF_BOOT) -o $@ $^

$(BINDIR)/boot.bin: $(OBJ_STAGE1) $(OBJ_COMMON) $(SRCDIR_BOOT_COMMON)/s1_bootdefs.lds
	$(LD) $(LD_ALL) $(LD_BOOT) $(LD_STAGE1) -o $@ $(OBJ_STAGE1) $(OBJ_COMMON)
