UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    CC=gcc-6 -fopenmp
endif

ifeq ($(UNAME_S),Linux)
    CC=gcc -fopenmp
endif

CFLAGS=-O3 -march=native -std=c99
EXEC=bench

.PHONY: all clean

all:
	${CC} atomic.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


