LD_STAGE1=-T $(SRCDIR_STAGE1)/linker.lds
AF_BOOT=-f elf32 -I $(SRCDIR_STAGE1)/

$(SRCDIR_STAGE1)/linker.lds: $(SRCDIR_BOOT_COMMON)/bootdefs.lds.S
	$(CC) $(CF_DEP) $(INCLUDE_PATH) -DLD_BOOT_STAGE1 -E -P -x c -o $@ $<

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_STAGE1)/%.s | $(OBJDIR_STAGE1)
	$(AS) $(AF_BOOT) -o $@ $<

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_BOOT_COMMON)/%.s | $(OBJDIR_STAGE1)
	$(AS) $(AF_BOOT) -o $@ $<

$(BINDIR)/boot.bin: $(SRCDIR_STAGE1)/linker.lds $(OBJ_STAGE1) $(OBJ_COMMON) | $(BINDIR)
	$(LD) $(LD_ALL) $(LD_BOOT) $(LD_STAGE1) -o $@ $(OBJ_STAGE1) $(OBJ_COMMON)
