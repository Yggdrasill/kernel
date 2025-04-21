SRC_LIBK=$(wildcard $(SRCDIR_LIBK)/*.c)
OBJ_LIBK=$(patsubst %.c,%.o,$(patsubst $(SRCDIR_LIBK)%,$(OBJDIR_LIBK)%,$(SRC_LIBK) ) )

AF_LIBK=-f elf32 -I libk/
LD_LIBK=--oformat elf32-i386 -r

$(OBJDIR)/libk.o: $(OBJ_LIBK) $(OBJDIR_LIBK)/asm_irq.o $(OBJDIR_LIBK)/asm_exception.o
	$(LD) $(LD_ALL) $(LD_LIBK) -o $@ $^

$(OBJDIR_LIBK)/%.o: ${SRCDIR_LIBK}/%.c ${SRCDIR_LIBK}/%.h
	$(CC) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $(filter-out %.h,$^)

$(OBJDIR_LIBK)/asm_irq.o: libk/irq.s
	$(AS) $(AF_LIBK) -o $@ $^

$(OBJDIR_LIBK)/asm_exception.o: libk/exception.s
	$(AS) $(AF_LIBK) -o $@ $^
