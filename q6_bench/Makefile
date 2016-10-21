CC=gcc
CFLAGS=-O1 -march=native -std=c99
EXEC=bench

.PHONY: all clean

all:
	${CC} q6_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


