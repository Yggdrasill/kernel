SRC_KLIBC=$(wildcard $(SRCDIR_KLIBC)/*.c)
OBJ_KLIBC=$(patsubst %.c,%.o,$(patsubst $(SRCDIR_KLIBC)%,$(OBJDIR_KLIBC)%,$(SRC_KLIBC)))

LD_KLIBC=--oformat elf32-i386 -r

$(OBJDIR_KLIBC)/%.o: $(SRCDIR_KLIBC)/%.c | $(OBJDIR_KLIBC)
	$(CC) $(CF_DEP) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/klibc.o: $(OBJ_KLIBC) | $(OBJDIR_KLIBC)
	$(LD) $(LD_ALL) $(LD_KLIBC) -o $@ $^

