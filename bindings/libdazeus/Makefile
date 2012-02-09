lib/libdazeus.a: src/libdazeus.c include/libdazeus.h
	$(CC) -c -o lib/libdazeus.a src/libdazeus.c -Iinclude -Wall

examples/networks: examples/networks.c include/libdazeus.h lib/libdazeus.a
	$(CC) -o examples/networks examples/networks.c lib/libdazeus.a -Iinclude -Wall
