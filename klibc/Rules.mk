SRCDIR_KLIBC=klibc
SRC_KLIBC=$(wildcard $(SRCDIR_KLIBC)/*.c)
OBJ_KLIBC=$(patsubst %.c,%.o,$(patsubst $(SRCDIR_KLIBC)%,$(OBJDIR)%,$(SRC_KLIBC) ) )

LD_KLIBC=--oformat elf32-i386 -r

$(OBJDIR)/klibc.o: $(OBJ_KLIBC)
	$(LD) $(LD_ALL) $(LD_KLIBC) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR_KLIBC)/%.c
	$(CC) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $^
