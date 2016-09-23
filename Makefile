
.PHONY: plots clean

plots:
	scripts/plot-q6
	scripts/plot-q6-costs
	scripts/plot-swapped-loops
	scripts/plot-swapped-loops-costs

clean:
	rm -rf raw/ *.pyc *.pdf plots/
