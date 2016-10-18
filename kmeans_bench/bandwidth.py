
SIZEOF_INT = 4.
SIZEOF_FLT = 8.

def to_gbps(bw):
    return (bw * 8) / 10E9

def membandwidth(n, m, k, i, time):
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
