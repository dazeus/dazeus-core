CFLAGS = -Wall -Iinclude -Icontrib/libjson $(EXTRACFLAGS)

lib/libdazeus.a: lib/libdazeus.o contrib/libjson/libjson.a
	cp contrib/libjson/libjson.a lib/libdazeus.a
	ar cur lib/libdazeus.a lib/libdazeus.o

contrib/libjson/libjson.a:
	make -C contrib/libjson libjson.a

lib/libdazeus.o: src/libdazeus.c include/libdazeus.h
	$(CC) -c -o lib/libdazeus.o src/libdazeus.c $(CFLAGS)

examples/networks: examples/networks.c include/libdazeus.h lib/libdazeus.a
	$(CXX) -o examples/networks examples/networks.c lib/libdazeus.a $(CFLAGS)

.PHONY : clean distclean
clean:
	rm -f lib/libdazeus.o
	make -C contrib/libjson clean

distclean: clean
	rm -f lib/libdazeus.a
