UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    CC=gcc-6 -fopenmp -std=c99
endif

ifeq ($(UNAME_S),Linux)
    CC=gcc -fopenmp
endif

CFLAGS=-O3 -march=native
EXEC=bench

.PHONY: all clean

all:
	${CC} kmeans_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


