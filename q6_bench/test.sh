#!/bin/bash
for v in 1 2 3 4 5; do
    echo ----- $v ------
    for s in 0.01 0.1 0.25 0.5 0.75 0.9 1.0; do
        ./bench -v $v -s $s
    done
done
