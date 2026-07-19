SRC_STAGE2:=$(filter-out $(SRCDIR_STAGE2)/mmap_gen.c $(SRCDIR_STAGE2)/linker_gen.c,$(wildcard $(SRCDIR_STAGE2)/*.c))
SRC_STAGE2:=$(SRC_STAGE2) \
			$(filter-out $(SRCDIR_STAGE2)/mmap_generated.s,$(wildcard $(SRCDIR_STAGE2)/*.s))
OBJ_STAGE2=$(patsubst %.s,%.o,$(patsubst %.c,%.o,$(patsubst $(SRCDIR_STAGE2)%,$(OBJDIR_STAGE2)%,$(SRC_STAGE2))))

LD_STAGE2=-T $(OBJDIR_STAGE2)/linker.lds

$(OBJDIR_GEN)/mmap_gen: $(SRCDIR_STAGE2)/mmap_gen.c | $(OBJDIR_GEN)
	$(CC) $(CF_HOST) -MMD -MP -MF $@.d -MT $@ -I ./ -DCC_HOSTED -DGEN_ASM_STAGE2 -o $@ $<

$(OBJDIR_GEN)/mmap_generated.s: $(OBJDIR_GEN)/mmap_gen
	$< > $@

$(OBJDIR_GEN)/linker_gen: $(SRCDIR_STAGE2)/linker_gen.c | $(OBJDIR_GEN)
	$(CC) $(CF_HOST) -MMD -MP -MF $@.d -MT $@ -I ./ -DCC_HOSTED -DGEN_LD_STAGE2 -o $@ $<

$(OBJDIR_GEN)/mmap_generated.h: $(OBJDIR_GEN)/linker_gen
	$< > $@

$(OBJDIR_STAGE2)/linker.lds: $(SRCDIR_BOOT_COMMON)/bootdefs.lds.S \
							 $(OBJDIR_GEN)/mmap_generated.h | $(OBJDIR_STAGE2)
	$(CC) -MMD -MP -MF $@.d -MT $@ $(INCLUDE_PATH) -DLD_BOOT_STAGE2 -E -P -x c -o $@ $<

$(OBJDIR_STAGE2)/mmap.o: $(SRCDIR_STAGE2)/mmap.s $(OBJDIR_GEN)/mmap_generated.s \
						 | $(OBJDIR_STAGE2)
	$(AS) -f elf32 -i $(OBJDIR_GEN) -o $@ $<

$(OBJDIR_STAGE2)/%.o: $(SRCDIR_STAGE2)/%.s | $(OBJDIR_STAGE2)
	$(AS) -f elf32 -o $@ $<

$(OBJDIR_STAGE2)/%.o: $(SRCDIR_STAGE2)/%.c | $(OBJDIR_STAGE2)
	$(CC) $(CF_DEP) $(CF_ALL) $(INCLUDE_PATH) -I $(SRCDIR_STAGE2)/ $(CFLAGS) -c -o $@ $<

$(BINDIR)/stage2.elf: $(OBJDIR_STAGE2)/linker.lds $(OBJDIR)/libk.o $(OBJDIR)/klibc.o \
					  $(OBJ_STAGE2) $(OBJ_STAGE1) $(OBJ_COMMON) | $(BINDIR)
	$(LD) $(LD_ALL) $(LD_BOOT) $(LD_STAGE2) -o $@ $(filter-out $(OBJDIR_STAGE2)/linker.lds,$^)
