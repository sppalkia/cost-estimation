CC=clang
CFLAGS=-O0 -march=native
EXEC=bench

.PHONY: all clean

all:
	${CC} q6_bench.c ${CFLAGS} -S -emit-llvm

clean:
	rm -f ${EXEC}


