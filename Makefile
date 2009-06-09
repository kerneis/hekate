CPC=cpc.asm.exe

CLIBS=-lcurl
CDEBUGFLAGS=-O3 -Wall -g
CFLAGS=$(CDEBUGFLAGS) $(CLIBS) $(EXTRA_DEFINES)


LDLIBS=-lcurl -lm -pthread -lcpcfull

.SUFFIXES: .cpc .cpi

.PHONY: all clean

all: main

main: sha1.o util.o io.o list.o hashtable.o parse.o torrent.o tracker.o server.o main.o

clean:
	rm -f *.o *~ *.cpi
	for x in *.cpc; do rm -f $${x%.cpc}.c; done;
	rm -f main

.cpc.cpi:
	gcc -E -x c $(CFLAGS) -include cpc/cpc_runtime.h \
	  -o $@ $<
.cpi.c:
	$(CPC) $< --out $@
