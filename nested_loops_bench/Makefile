CC=clang
CFLAGS=-O0 -march=native
EXEC=bench

.PHONY: all clean

all:
	${CC} nested_loop_bench.c ${CFLAGS} -o bench

clean:
	rm -f ${EXEC}


