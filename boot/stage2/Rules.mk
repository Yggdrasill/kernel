LD_STAGE2=-T $(SRCDIR_STAGE2)/linker.ld

$(OBJDIR_STAGE2)/start.o: $(SRCDIR_STAGE2)/start.s
	$(AS) -f elf32 -o $@ $^

$(OBJDIR_STAGE2)/mmap.o: $(SRCDIR_STAGE2)/mmap.s
	$(AS) -f elf32 -o $@ $^

$(OBJDIR_STAGE2)/stage2.o: $(SRCDIR_STAGE2)/stage2.c 
	$(CC) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $^

$(BINDIR)/stage2.elf: $(OBJDIR)/libk.o $(OBJDIR)/klibc.o $(OBJDIR_STAGE2)/stage2.o $(OBJDIR_STAGE2)/start.o $(OBJDIR_STAGE2)/mmap.o $(OBJ_STAGE1) $(OBJ_COMMON)
	$(LD) $(LD_ALL) $(LD_STAGE2) -o $@ $^
