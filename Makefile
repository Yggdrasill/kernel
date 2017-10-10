all:
	+${MAKE} -C boot/

clean:
	rm boot/*.bin
	rm boot/*.o
