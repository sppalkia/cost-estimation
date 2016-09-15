#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for v in 1 2 3 4 5; do
    echo ----- $v ------
    for s in 0.01 0.1 0.25 0.5 0.75 0.9 1.0; do
        $CBO/q6_bench/bench -v $v -s $s
    done
done
