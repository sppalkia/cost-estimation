
.PHONY: plots clean

plots:
	scripts/plot-q6
	scripts/plot-q6-costs
	scripts/plot-nested-loops
	scripts/plot-nested-loops-costs

clean:
	rm -rf *_raw/ *.pyc *.pdf
