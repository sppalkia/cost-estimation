
.PHONY: plots clean

plots:
	scripts/plot-q6
	scripts/plot-q6-costs

clean:
	rm -rf *_raw/ *.pyc *.pdf
