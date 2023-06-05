LD_STAGE2=-T boot/linker.ld

AF_BOOT=-f elf32 -I boot/

$(OBJDIR)/boot.o: boot/boot.s
	$(AS) $(AF_BOOT) -o $@ $^

$(OBJDIR)/start.o: boot/start.s
	$(AS) -f elf32 -o $@ $^

$(OBJDIR)/stage2.o: boot/stage2.c
	$(CC) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $^

$(BINDIR)/boot.bin: $(OBJDIR)/libk.o $(OBJDIR)/klibc.o $(OBJDIR)/stage2.o $(OBJDIR)/boot.o $(OBJDIR)/start.o
	$(LD) $(LD_ALL) $(LD_STAGE2) -o $@ $^
