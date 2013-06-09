CPC=cpc

CDEBUGFLAGS=-O2 -Wall -Wno-uninitialized -g

CLIBS+=`curl-config --libs`
CFLAGS+= `curl-config --cflags` $(CDEBUGFLAGS) $(EXTRA_DEFINES)

LDLIBS+=`curl-config --libs` -lcrypto -pthread -lcpcfull -lm

.SUFFIXES: .cpc .cpi

.PHONY: all clean

hekate: util.o io.o list.o hashtable.o parse.o torrent.o tracker.o server.o hekate.o dht/dht.o

all: hekate

clean:
	rm -f *.o *~ dht/dht.o
	rm -f hekate

.cpc.o:
	$(CPC) $(CPCOPTS) $(CFLAGS) $(CLIBS) -c -o $@ $<

