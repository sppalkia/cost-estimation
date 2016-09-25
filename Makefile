
.PHONY: plots clean

plots:
	scripts/plot-q6
	scripts/plot-q6-costs
	scripts/plot-swapped-loops
	scripts/plot-swapped-loops-costs
	scripts/plot-matrix-multiplication
	scripts/plot-matrix-multiplication-costs

clean:
	rm -rf *_raw/ *.pyc *.pdf plots/
