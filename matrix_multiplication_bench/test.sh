#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for it in 1 2 3 4 5; do
    for n in 128 256 512 1024 2048 3072 4096; do
        $CBO/matrix_multiplication_bench/bench -n $n
    done
done
