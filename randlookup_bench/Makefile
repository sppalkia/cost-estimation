CC=clang
CFLAGS=-O0 -march=native
EXEC=bench

.PHONY: all clean

all:
	${CC} randlookup_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


