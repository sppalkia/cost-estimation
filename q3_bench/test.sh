#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for it in 1 2 3 4 5; do 
    for b in 10 1000 10000 100000 1000000 10000000 100000000; do
        echo ----- $b -----
        for p in 0.01 0.1 0.5 0.75 1.0; do
            $CBO/q3_bench/bench -b $b -p $p
        done
    done
done
