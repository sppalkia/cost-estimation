#!/usr/bin/env python

SIZEOF_INT = 4.
SIZEOF_FLT = 8.

def to_gbps(bw):
    """
    Convert bytes / sec to Gigabits/sec.
    """
    return (bw * 8) / 10E9

def membandwidth(n, m, k, i, time):
    """
    Computes the memory bandwidth for the k-means
    algorithm given n (points), m (dimension), k (centers),
    i (number of iterations), and the runtime in seconds.
    """
    mem = 0.
    # Reset counts
    mem += SIZEOF_INT * k
    # Reset temp centroids
    mem += SIZEOF_FLT * m * k
    # Distance computation
    mem +=  n * m * (k + 1) * SIZEOF_FLT
    # Access Label
    mem += n * SIZEOF_INT
    # Update temp. centroid
    mem += 2 * m * n * SIZEOF_FLT
    # Update centroids
    mem += 2 * k * m * SIZEOF_FLT
    # Iterations
    mem *= i
    return mem / time

def parse_file(fn):
    with open(fn) as f:
        for line in f:
            if ">>>" in line:
                blocked, tokens = line.split(":")
                n, m, k, i, time, _ = [float(x) for x in tokens.strip().split(",")]
                bw = to_gbps(membandwidth(n, m, k, i, time))
                print "{6}, n={0}, m={1}, k={2}, i={3}, time={4}, bw={5}".format(
                        n, m, k, i, time, bw, blocked)

import os

if __name__=="__main__":
    cbo = os.environ["CBO"]
    filename = cbo + "/raw/kmeans_bench/raw.out"
    parse_file(filename)
