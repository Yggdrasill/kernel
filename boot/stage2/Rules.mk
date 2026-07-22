SRC_STAGE2:=$(filter-out $(SRCDIR_STAGE2)/symbol_gen.c,$(wildcard $(SRCDIR_STAGE2)/*.c))
SRC_STAGE2:=$(SRC_STAGE2) $(wildcard $(SRCDIR_STAGE2)/*.s)
OBJ_STAGE2=$(patsubst %.s,%_asm.o,$(patsubst %.c,%.o,\
		   $(patsubst $(SRCDIR_STAGE2)%,$(OBJDIR_STAGE2)%,$(SRC_STAGE2))))

LD_STAGE2=-T $(OBJDIR_STAGE2)/linker.lds

$(OBJDIR_GEN)/symbol_gen: $(SRCDIR_STAGE2)/symbol_gen.c | $(OBJDIR_GEN)
	$(CC) $(CF_HOST) -no-pie -O0 -MMD -MP -MF $@.d -MT $@ \
		-I ./ -o $@ $< -Wl,--no-gc-sections,-Ttext-segment=0

$(OBJDIR_GEN)/mmap_generated.s: $(OBJDIR_GEN)/symbol_gen
	for sym in $$(readelf -Ws $< | grep "ABI_\(MMAP\|INFO\)" \
		| awk -v OFS=',' '{ print $$8,$$3,$$2 };'); \
	do \
		name=$${sym%%,*}; \
		tail=$${sym#*,}; \
		size=$${tail%%,*}; \
		offset=$${tail#*,}; \
		value=$$(xxd -l $${size} -e -s "0x$${offset}" $< | awk '{ print $$2 }'); \
		echo "$${name} equ 0x$${value}"; \
	done > $@

$(OBJDIR_GEN)/mmap_generated.h: $(OBJDIR_GEN)/symbol_gen
	for sym in $$(readelf -Ws $< | grep "ABI_\(MMAP\|INFO\|[GI]DT\|LINK\)" \
		| awk -v OFS=',' '{ print $$8,$$3,$$2 };'); \
	do \
		name=$${sym%%,*}; \
		tail=$${sym#*,}; \
		size=$${tail%%,*}; \
		offset=$${tail#*,}; \
		value=$$(xxd -l $${size} -e -s "0x$${offset}" $< | awk '{ print $$2 }'); \
		echo "#define $${name} 0x$${value}"; \
	done > $@

$(OBJDIR_STAGE2)/linker.lds: $(SRCDIR_BOOT_COMMON)/linker.lds.S \
							 $(OBJDIR_GEN)/mmap_generated.h | $(OBJDIR_STAGE2)
	$(CC) -MMD -MP -MF $@.d -MT $@ $(INCLUDE_PATH) -DLD_BOOT_STAGE2 -E -P -x c -o $@ $<

$(OBJDIR_STAGE2)/mmap_asm.o: $(SRCDIR_STAGE2)/mmap.s $(OBJDIR_GEN)/mmap_generated.s \
						     | $(OBJDIR_STAGE2)
	$(AS) -f elf32 -i $(OBJDIR_GEN) -o $@ $<

$(OBJDIR_STAGE2)/%_asm.o: $(SRCDIR_STAGE2)/%.s | $(OBJDIR_STAGE2)
	$(AS) -f elf32 -o $@ $<

$(OBJDIR_STAGE2)/%.o: $(SRCDIR_STAGE2)/%.c | $(OBJDIR_STAGE2)
	$(CC) $(CF_DEP) $(CF_ALL) $(INCLUDE_PATH) -I $(SRCDIR_STAGE2)/ $(CFLAGS) -c -o $@ $<

$(BINDIR)/stage2.elf: $(OBJDIR_STAGE2)/linker.lds $(OBJDIR)/libk.o \
					  $(OBJDIR)/klibc.o $(OBJ_STAGE2) $(OBJ_STAGE1) \
					  $(OBJ_COMMON) | $(BINDIR)
	$(LD) $(LD_ALL) $(LD_BOOT) $(LD_STAGE2) -o $@ $(filter-out $(OBJDIR_STAGE2)/linker.lds,$^)
