SRC_STAGE2=$(wildcard $(SRCDIR_STAGE2)/*.c) $(wildcard $(SRCDIR_STAGE2)/*.s)
OBJ_STAGE2=$(patsubst %.s,%.o,$(patsubst %.c,%.o,$(patsubst $(SRCDIR_STAGE2)%,$(OBJDIR_STAGE2)%,$(SRC_STAGE2))))

LD_STAGE2=-T $(SRCDIR_STAGE2)/linker.lds

$(SRCDIR_STAGE2)/linker.lds: $(SRCDIR_BOOT_COMMON)/bootdefs.lds.S
	$(CC) $(CF_DEP) $(INCLUDE_PATH) -DLD_BOOT_STAGE2 -E -P -x c -o $@ $<

$(OBJDIR_STAGE2)/%.o: $(SRCDIR_STAGE2)/%.s | $(OBJDIR_STAGE2)
	$(AS) -f elf32 -o $@ $<

$(OBJDIR_STAGE2)/%.o: $(SRCDIR_STAGE2)/%.c | $(OBJDIR_STAGE2)
	$(CC) $(CF_DEP) $(CF_ALL) $(INCLUDE_PATH) -I $(SRCDIR_STAGE2)/ $(CFLAGS) -c -o $@ $<

$(BINDIR)/stage2.elf: $(SRCDIR_STAGE2)/linker.lds $(OBJDIR)/libk.o $(OBJDIR)/klibc.o $(OBJ_STAGE2) $(OBJ_STAGE1) $(OBJ_COMMON) | $(BINDIR)
	$(LD) $(LD_ALL) $(LD_BOOT) $(LD_STAGE2) -o $@ $(filter-out $(SRCDIR_STAGE2)/linker.lds,$^)
