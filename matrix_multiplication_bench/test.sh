#!/bin/bash

if [ -z "$CBO" ]; then
    echo "Set CBO to root of this repo."
    exit 1
fi  

for it in 1; do
  for b in 64 128 256; do
    echo ----- $b -----
    for n in 128 256 512 1024 2048; do
        $CBO/matrix_multiplication_bench/bench -n $n -b $b
    done
  done
done
