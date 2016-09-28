CC=clang
CFLAGS=-O0 -march=native
EXEC=bench

.PHONY: all clean

all:
	${CC} matrix_multiplication_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


