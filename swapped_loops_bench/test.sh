#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for it in 1 2 3 4 5; do
    for k in 10 100 500 800 1000; do
        echo ----- $k -----
        for n in 10000; do
            $CBO/swapped_loops_bench/bench -k $k -n $n
        done
    done
done
