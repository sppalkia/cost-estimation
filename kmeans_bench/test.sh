#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

# n: number of points
# k: number of centroids
# m: dimension of data
for n in 256 512 1024 2048 4096 8192 16384; do
    for k in 2 4 256 512 1024 2048; do
        echo ----- $k -----
        for m in 2 4 8 128 256 512 1024 2048; do
            $CBO/kmeans_bench/bench -n $n -k $k -m $m -i 5
        done
    done
done
