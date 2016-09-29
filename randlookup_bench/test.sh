#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for _ in {1..5}; do
    for k in 100 10000 100000 1000000 10000000 100000000; do
        echo ----- $k -----
        $CBO/randlookup_bench/bench -k $k
    done
done
