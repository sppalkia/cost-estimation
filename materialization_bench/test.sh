#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for n in 100 1000 10000 100000 1000000 10000000 100000000 1000000000; do
    echo ----- $n -----
    $CBO/materialization_bench/bench -n $n
done
