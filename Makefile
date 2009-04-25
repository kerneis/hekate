#CPC_DIR=/home/kerneis/cpc/stable
#CPC_DIR=/home/yoann/sources/cpc
#CPC=$(CPC_DIR)/bin/cpc.native

CLIBS=-I -lcurl #$(CPC_DIR)/runtime
CFLAGS=-O3 -Wall -g $(CLIBS)

LDLIBS=-lcurl #$(CPC_DIR)/runtime/libcpc.a $(CPC_DIR)/runtime/cpc_runtime.o -pthread

#.SUFFIXES: .cpc .cpi

.PHONY: all clean

all: main

main: sha1.o list.o hashtable.o parse.o tracker_conn.o server.o main.o

clean:
	rm -f *.o *~ *.cpi
	rm -f main

#.cpc.cpi:
#	$(CPP) -P $(CFLAGS) -include cpc_runtime.h \
#	  - <$< >$@
#.cpi.c:
#	$(CPC) $< --out $@
