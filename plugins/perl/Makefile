
PERL=/sw/bin/perl5.8.8
PERLSRCDIR=`$(PERL) -MConfig -e 'print $$Config{archlib}'`/CORE
CXX=g++
CC=gcc
CFLAGS=-arch i386
LDFLAGS=-arch i386

all: DaZeus2.o embedperl.o xsinit.o test

test: DaZeus2.o embedperl.o xsinit.o main.cpp
	$(CXX) $(LDFLAGS) $(CFLAGS) -o test main.cpp DaZeus2.o embedperl.o xsinit.o -I "$(PERLSRCDIR)" \
    -L"$(PERLSRCDIR)" -lperl `$(PERL) -MExtUtils::Embed -e ldopts`

DaZeus2.c: DaZeus2.xs
	xsubpp DaZeus2.xs >DaZeus2.c

DaZeus2.o: DaZeus2.c
	$(CC) $(CFLAGS) -c -o DaZeus2.o -I $(PERLSRCDIR) DaZeus2.c

embedperl.o: embedperl.cpp
	$(CXX) $(CFLAGS) -c -o embedperl.o embedperl.cpp -I $(PERLSRCDIR) -Dbool=char -DHAS_BOOL

xsinit.c:
	$(PERL) -MExtUtils::Embed -e xsinit -- -o xsinit.c -std DaZeus2

xsinit.o: xsinit.c
	$(CC) $(CFLAGS) -c xsinit.c `$(PERL) -MExtUtils::Embed -e ccopts`

clean:
	rm -f test DaZeus2.c DaZeus2.o embedperl.o xsinit.c xsinit.o

