CC=gcc
CFLAGS=-O3 -march=native -std=c99
EXEC=bench

.PHONY: all clean

all:
	${CC} randlookup_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


