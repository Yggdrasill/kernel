LD_STAGE2=-T boot/linker.ld

$(BINDIR)/boot.bin: boot/boot.s
	$(AS) -f bin -i boot/ -o $@ $^

$(OBJDIR)/stage2.o: boot/stage2.c
	$(CC) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $^

$(BINDIR)/stage2.bin: $(OBJDIR)/libk.o $(OBJDIR)/klibc.o $(OBJDIR)/stage2.o
	$(LD) $(LD_ALL) $(LD_STAGE2) -o $@ $^
