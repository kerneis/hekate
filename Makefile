CPC=cpc.asm.exe

CDEBUGFLAGS=-O3 -Wall -Wno-uninitialized

CLIBS+=`curl-config --libs`
CFLAGS+= `curl-config --cflags` $(CDEBUGFLAGS) $(EXTRA_DEFINES)

LDLIBS+=`curl-config --libs` -lssl -lm -pthread -lcpcfull

.SUFFIXES: .cpc .cpi

.PHONY: all clean

hekate: util.o io.o list.o hashtable.o parse.o torrent.o tracker.o server.o hekate.o dht/dht.o

all: hekate

clean:
	rm -f *.o *~ *.cpi dht/dht.o
	for x in *.cpc; do rm -f $${x%.cpc}.c; done;
	rm -f hekate

.cpc.cpi:
	$(CC) -E -x c $(CFLAGS) $(CLIBS) -o $@ $<

.cpi.c:
	$(CPC) $(CPCOPTS) $< --out $@
