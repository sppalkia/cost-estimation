CC=gcc -fopenmp -std=c99
CFLAGS=-O3 -march=native
EXEC=bench

.PHONY: all clean

all:
	${CC} kmeans_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


