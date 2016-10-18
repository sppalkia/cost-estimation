#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

# n: number of points
# k: number of centroids
# m: dimension of data
for n in 8192 16384 32768 65536 131072; do
    for k in 2 4 256 512 1024 2048; do
        for m in 2 4 8 128 256 512 1024 2048; do
            $CBO/kmeans_bench/bench -n $n -k $k -m $m -i 1
        done
    done
done
