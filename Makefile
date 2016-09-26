
.PHONY: benchmark-plots cost-plots all-plots clean

benchmark-plots:
	scripts/plot-q6
	scripts/plot-swapped-loops
	# scripts/plot-matrix-multiplication

cost-plots:
	scripts/plot-q6-costs
	scripts/plot-swapped-loops-costs
	scripts/plot-matrix-multiplication-costs

all-plots: benchmark-plots cost-plots

clean:
	rm -rf *_raw/ *.pyc *.pdf plots/
