$(OBJDIR)/klibc.o: klibc/*.c
	$(CC) $(CF_ALL) $(INCLUDE_PATH) $(CFLAGS) -c -o $@ $^
