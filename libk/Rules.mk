SRCDIR_LIBK=libk
SRC_LIBK=$(wildcard $(SRCDIR_LIBK)/*.c)
OBJ_LIBK=$(patsubst %.c,%.o,$(patsubst $(SRCDIR_LIBK)%,$(OBJDIR)%,$(SRC_LIBK) ) )

AF_LIBK=-f elf32 -i libk/
LD_LIBK=--oformat elf32-i386 -r

$(OBJDIR)/libk.o: $(OBJ_LIBK) $(OBJDIR)/asm_irq.o $(OBJDIR)/asm_exception.o
	$(LD) $(LD_ALL) $(LD_LIBK) -o $@ $^

$(OBJDIR)/%.o: libk/%.c
	$(CC) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/asm_irq.o: libk/irq.s
	$(AS) $(AF_LIBK) -o $@ $^

$(OBJDIR)/asm_exception.o: libk/exception.s
	$(AS) $(AF_LIBK) -o $@ $^
