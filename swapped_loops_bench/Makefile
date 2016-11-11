CC=gcc
CFLAGS=-O3 -march=native -std=c99
EXEC=bench

.PHONY: all clean

all:
	${CC} swapped_loops_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


