SRC_LIBK=$(wildcard $(SRCDIR_LIBK)/*.c) $(wildcard $(SRCDIR_LIBK)/*.s)
OBJ_LIBK=$(patsubst %.s,%_asm.o,$(patsubst %.c,%.o,$(patsubst $(SRCDIR_LIBK)%,$(OBJDIR_LIBK)%,$(SRC_LIBK))))

AF_LIBK=-f elf32 -I libk/
LD_LIBK=--oformat elf32-i386 -r

$(OBJDIR_LIBK)/%_asm.o: $(SRCDIR_LIBK)/%.s | $(OBJDIR_LIBK)
	$(AS) $(AF_LIBK) -o $@ $<

$(OBJDIR_LIBK)/%.o: ${SRCDIR_LIBK}/%.c | $(OBJDIR_LIBK)
	$(CC) $(CF_DEP) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/libk.o: $(OBJ_LIBK) | $(OBJDIR_LIBK)
	$(LD) $(LD_ALL) $(LD_LIBK) -o $@ $^
