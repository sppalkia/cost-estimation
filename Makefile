
.PHONY: make-benchmarks benchmark-plots cost-plots all-plots clean

all: benchmark-plots cost-plots

make-benchmarks:
	make --directory q6_bench
	make --directory swapped_loops_bench
	make --directory matrix_multiplication_bench
	make --directory randlookup_bench
	make --directory q1_bench

benchmark-plots: make-benchmarks
	scripts/plot-q6
	scripts/plot-swapped-loops
	scripts/plot-matrix-multiplication
	scripts/plot-randlookup
	scripts/plot-q1

cost-plots:
	scripts/plot-q6-costs
	scripts/plot-swapped-loops-costs
	scripts/plot-matrix-multiplication-costs
	scripts/plot-randlookup-costs
	scripts/plot-q1-costs

all-plots: benchmark-plots cost-plots

clean:
	make clean --directory q6_bench
	make clean --directory swapped_loops_bench
	make clean --directory matrix_multiplication_bench
	make clean --directory randlookup_bench
	make clean --directory q1_bench
	rm -rf raw/ *.pyc *.pdf plots/
