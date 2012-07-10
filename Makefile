CPC_FOLDER  = ../cpc
CPC         = $(CPC_FOLDER)/bin/cpc.asm.exe
CPC_INCLUDE = -I $(CPC_FOLDER)/include
CPC_LIB     = -L $(CPC_FOLDER)/runtime

OPENSSL_INCLUDE = -I C:/OpenSSL/mingw32/include
OPENSSL_LIB     = -L C:/OpenSSL/mingw32/lib

CDEBUGFLAGS=-O2 -Wall -Wno-uninitialized -g

CLIBS += $(OPENSSL_LIB) \
         $(CPC_LIB)
CFLAGS += $(OPENSSL_INCLUDE) \
          $(CPC_INCLUDE) $(CDEBUGFLAGS) $(EXTRA_DEFINES)

LDLIBS += $(CLIBS) -lcpcfull -lm -lWs2_32 -lMswsock -lcrypto

.SUFFIXES: .cpc .cpi

.PHONY: all clean

hekate: util.o io.o list.o hashtable.o parse.o torrent.o tracker.o server.o \
        hekate.o dht/dht.o dht/inet_ntop.o

all: hekate

clean:
	rm -f *.o *~ *.cpi dht/dht.o
	for x in *.cpc; do rm -f $${x%.cpc}.c; done;
	rm -f hekate

.cpc.cpi:
	$(CC) -E -x c $(CFLAGS) $(CLIBS) -o $@ $<

.cpi.c:
	$(CPC) $(CPCOPTS) $< --out $@
