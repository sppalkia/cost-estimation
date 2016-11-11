#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for it in 1 2 3 4 5; do
    for p in 0.00001 0.01 0.5 1.0; do 
        for k in 10 100 1000 10000 1000000; do
        echo ----- $k -----
        $CBO/swapped_loops_bench/bench -k $k -p $p -n 10000
        done
    done
done
