CFLAGS = -Wall -Iinclude -Icontrib/libjson $(EXTRACFLAGS)

lib/libdazeus.a: lib/libdazeus.o contrib/libjson/libjson.a
	mkdir -p lib
	cp contrib/libjson/libjson.a lib/libdazeus.a
	ar cur lib/libdazeus.a lib/libdazeus.o

contrib/libjson/libjson.a:
	make -C contrib/libjson libjson.a

lib/libdazeus.o: src/libdazeus.c include/libdazeus.h
	$(CC) -c -o lib/libdazeus.o src/libdazeus.c $(CFLAGS)

examples/networks: examples/networks.c include/libdazeus.h lib/libdazeus.a
	$(CXX) -o examples/networks examples/networks.c lib/libdazeus.a $(CFLAGS)

examples/counter: examples/counter.c include/libdazeus.h lib/libdazeus.a
	$(CXX) -o examples/counter examples/counter.c lib/libdazeus.a $(CFLAGS)

examples/counterreset: examples/counterreset.c include/libdazeus.h lib/libdazeus.a
	$(CXX) -o examples/counterreset examples/counterreset.c lib/libdazeus.a $(CFLAGS)

examples/monitor: examples/monitor.c include/libdazeus.h lib/libdazeus.a
	$(CXX) -o examples/monitor examples/monitor.c lib/libdazeus.a $(CFLAGS)

examples/scope: examples/scope.c include/libdazeus.h lib/libdazeus.a
	$(CXX) -o examples/scope examples/scope.c lib/libdazeus.a $(CFLAGS)

.PHONY : clean distclean
clean:
	rm -f lib/libdazeus.o
	rm -rf examples/networks.dSYM
	rm -rf examples/counter.dSYM
	rm -rf examples/counterreset.dSYM
	rm -rf examples/monitor.dSYM
	rm -rf examples/scope.dSYM
	make -C contrib/libjson clean

distclean: clean
	rm -f lib/libdazeus.a
	rm -f examples/networks examples/counter examples/counterreset
	rm -f examples/monitor examples/scope
