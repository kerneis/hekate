CPC=cpc.asm.exe

CDEBUGFLAGS=-O3 -Wall -Wno-uninitialized

CLIBS+=`curl-config --libs`
CFLAGS+= `curl-config --cflags` $(EXTRA_DEFINES)

LDLIBS+=`curl-config --libs` -lssl -lm -pthread -lcpcfull

.SUFFIXES: .cpc .cpi

.PHONY: all clean

hekate: util.o io.o list.o hashtable.o parse.o torrent.o tracker.o server.o hekate.o dht/dht.o

all: hekate

dht/dht.o: dht/dht.c
	cd dht && $(MAKE) dht.o

clean:
	cd dht && $(MAKE) clean
	rm -f *.o *~ *.cpi
	for x in *.cpc; do rm -f $${x%.cpc}.c; done;
	rm -f hekate

.cpc.cpi:
	$(CC) -E -x c $(CFLAGS) $(CLIBS) -o $@ $<

.cpi.c:
	$(CPC) $(CPCOPTS) $< --out $@
