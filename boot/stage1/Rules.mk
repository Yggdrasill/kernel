LD_STAGE1=-T $(OBJDIR_STAGE1)/linker.lds
AF_BOOT=-f elf32 -I $(SRCDIR_STAGE1)/

$(OBJDIR_STAGE1)/linker.lds: $(SRCDIR_BOOT_COMMON)/linker.lds.S | $(OBJDIR_STAGE1)
	$(CC) -MMD -MP -MF $@.d -MT $@ $(INCLUDE_PATH) -DLD_BOOT_STAGE1 -E -P -x c -o $@ $<

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_STAGE1)/%.s | $(OBJDIR_STAGE1)
	$(AS) $(AF_BOOT) -o $@ $<

$(OBJDIR_STAGE1)/%.o: $(SRCDIR_BOOT_COMMON)/%.s | $(OBJDIR_STAGE1)
	$(AS) $(AF_BOOT) -o $@ $<

$(OBJDIR_STAGE1)/boot.o: $(SRCDIR_STAGE1)/boot.s \
						 $(OBJDIR_GEN)/s1_generated.s | $(OBJDIR_STAGE1)
	$(AS) $(AF_BOOT) -i $(OBJDIR_GEN) -o $@ $<

$(OBJDIR_STAGE1)/disk.o: $(SRCDIR_BOOT_COMMON)/disk.s \
						 $(OBJDIR_GEN)/s1_generated.s | $(OBJDIR_STAGE1)
	$(AS) $(AF_BOOT) -i $(OBJDIR_GEN) -o $@ $<

$(BINDIR)/boot.bin: $(OBJ_STAGE1) $(OBJ_COMMON) $(OBJDIR_STAGE1)/linker.lds | $(BINDIR)
	$(LD) $(LD_ALL) $(LD_BOOT) $(LD_STAGE1) -o $@ $(OBJ_STAGE1) $(OBJ_COMMON)

$(OBJDIR_GEN)/s1_symbol_gen: $(SRCDIR_BOOT_COMMON)/symbol_gen.c | $(OBJDIR_GEN)
	$(CC) $(CF_ALL) $(INCLUDE_PATH) -no-pie -O0 -MMD -MP -MF $@.d -MT $@ \
		-I ./ -o $@ $< -Wl,--no-gc-sections,-Ttext-segment=0

$(OBJDIR_GEN)/s1_generated.s: $(OBJDIR_GEN)/s1_symbol_gen
	for sym in $$(readelf -Ws $< | grep "ABI_\(DISK\|STAGE2\)" \
		| awk -v OFS=',' '{ print $$8,$$3,$$2 };'); \
	do \
		name=$${sym%%,*}; \
		tail=$${sym#*,}; \
		size=$${tail%%,*}; \
		offset=$${tail#*,}; \
		value=$$(xxd -l $${size} -e -s "0x$${offset}" $< | awk '{ print $$2 }'); \
		echo "$${name} equ 0x$${value}"; \
	done > $@
