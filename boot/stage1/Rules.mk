LD_STAGE1=-T boot/stage1/linker.ld

AF_BOOT=-f elf32 -I boot/stage1/

$(OBJDIR)/boot.o: boot/stage1/boot.s
	$(AS) $(AF_BOOT) -o $@ $^

$(BINDIR)/boot.bin: $(OBJDIR)/boot.o
	$(LD) $(LD_ALL) $(LD_STAGE1) -o $@ $^
