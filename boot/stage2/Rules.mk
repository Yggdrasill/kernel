LD_STAGE2=-T $(SRCDIR_STAGE2)/linker.lds

$(SRCDIR_STAGE2)/linker.lds: $(SRCDIR_BOOT_COMMON)/bootdefs.lds.S
	$(CC) $(CF_DEP) $(INCLUDE_PATH) -DLD_BOOT_STAGE2 -E -P -x c -o $@ $^

$(OBJDIR_STAGE2)/start.o: $(SRCDIR_STAGE2)/start.s
	$(AS) -f elf32 -o $@ $^

$(OBJDIR_STAGE2)/mmap.o: $(SRCDIR_STAGE2)/mmap.s
	$(AS) -f elf32 -o $@ $^

$(OBJDIR_STAGE2)/stage2.o: $(SRCDIR_STAGE2)/stage2.c
	$(CC) $(CF_DEP) $(CF_ALL) $(INCLUDE_PATH) -I $(SRCDIR_STAGE2)/ $(CFLAGS) -c -o $@ $^

$(OBJDIR_STAGE2)/rmode.o: $(SRCDIR_STAGE2)/rmode.c $(SRCDIR_STAGE2)/rmode.h
	$(CC) $(CF_DEP) $(CF_ALL) $(INCLUDE_PATH) -I $(SRCDIR_STAGE2)/ $(CFLAGS) -c -o $@ $(filter-out $(SRCDIR_STAGE2)/%.h,$^)

$(BINDIR)/stage2.elf: $(SRCDIR_STAGE2)/linker.lds $(OBJDIR)/libk.o $(OBJDIR)/klibc.o $(OBJDIR_STAGE2)/start.o $(OBJDIR_STAGE2)/stage2.o $(OBJDIR_STAGE2)/rmode.o $(OBJDIR_STAGE2)/mmap.o $(OBJ_STAGE1) $(OBJ_COMMON)
	$(LD) $(LD_ALL) $(LD_BOOT) $(LD_STAGE2) -o $@ $(filter-out $(SRCDIR_STAGE2)/linker.lds,$^)
