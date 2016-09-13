CC=clang
CFLAGS=-O0 -march=native
EXEC=bench

.PHONY: all clean

all:
	${CC} q6_bench.c ${CFLAGS} -S -emit-llvm
	${CC} -o ${EXEC} q6_bench.ll

clean:
	rm -f q6_bench.ll ${EXEC}


