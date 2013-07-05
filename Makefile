CPC=cpc

CDEBUGFLAGS=-O2 -Wall -Wno-uninitialized -g

CLIBS+=`curl-config --libs`
CFLAGS+= `curl-config --cflags` $(CDEBUGFLAGS) $(EXTRA_DEFINES)

LDLIBS+=`curl-config --libs` -lcrypto -pthread -lcpc -lm

.SUFFIXES: .cpc .dot .pdf

.PHONY: all clean pdf

hekate: util.o io.o list.o hashtable.o parse.o torrent.o tracker.o server.o hekate.o dht/dht.o

all: hekate

pdf: tracker.pdf hekate.pdf server.pdf io.pdf

clean:
	rm -f *.o *.i *.cil.* *.pdf *~ dht/dht.o
	rm -f hekate

.cpc.o:
	$(CPC) $(CPCOPTS) $(CFLAGS) $(CLIBS) -c -o $@ $<

.cpc.dot:
	$(CPC) --docpsInference --save-temps $(CPCOPTS) $(CFLAGS) $(CLIBS) -c $<

.dot.pdf:
	dot -Tpdf -o $@ $<
