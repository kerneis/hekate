CPC_DIR=/home/pejman/developement/stable
CPC=$(CPC_DIR)/bin/cpc.asm.exe

CLIBS=-lcurl -I $(CPC_DIR)/runtime -I $(CPC_DIR)/libev
CFLAGS=-O3 -Wall -g $(CLIBS)

LDLIBS=-lcurl $(CPC_DIR)/runtime/libcpc.a $(CPC_DIR)/runtime/cpc_runtime.o -lm -pthread

.SUFFIXES: .cpc .cpi

.PHONY: all clean

all: main

main: sha1.o list.o hashtable.o parse.o tracker_io.o server.o main.o

clean:
	rm -f *.o *~ *.cpi
	rm -f main

.cpc.cpi:
	gcc -E -x c $(CFLAGS) -include cpc_runtime.h \
	  -o $@ $<
.cpi.c:
	$(CPC) $< --out $@
