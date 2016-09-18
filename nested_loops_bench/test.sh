#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for it in 1 2 3 4 5; do
    for k in 500; do
        echo ----- $k ------
        for n in 1000 10000 100000 1000000 1000000 10000000 10000000 100000000; do
            $CBO/nested_loops_bench/bench -k $k -n $n
        done
    done
done
