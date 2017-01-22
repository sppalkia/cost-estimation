CC=gcc
CFLAGS=-O1 -march=native -std=c99
EXEC=bench

.PHONY: all clean

all:
	${CC} materialization_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


