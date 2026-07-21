LD_STAGE1=-T $(OBJDIR_STAGE1)/linker.lds
AF_BOOT=-f elf32 -I $(SRCDIR_STAGE1)/

$(OBJDIR_STAGE1)/linker.lds: $(SRCDIR_BOOT_COMMON)/linker.lds.S | $(OBJDIR_STAGE1)
	$(CC) -MMD -MP -MF $@.d -MT $@ $(INCLUDE_PATH) -DLD_BOOT_STAGE1 -E -P -x c -o $@ $<

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_STAGE1)/%.s | $(OBJDIR_STAGE1)
	$(AS) $(AF_BOOT) -o $@ $<

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_BOOT_COMMON)/%.s | $(OBJDIR_STAGE1)
	$(AS) $(AF_BOOT) -o $@ $<

$(BINDIR)/boot.bin: $(OBJ_STAGE1) $(OBJ_COMMON) $(OBJDIR_STAGE1)/linker.lds | $(BINDIR)
	$(LD) $(LD_ALL) $(LD_BOOT) $(LD_STAGE1) -o $@ $(OBJ_STAGE1) $(OBJ_COMMON)
