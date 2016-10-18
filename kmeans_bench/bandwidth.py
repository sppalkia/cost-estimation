
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

print to_gbps(membandwidth(65536, 1024, 4096, 1, 121.516))
print to_gbps(membandwidth(65536, 1024, 4096, 1, 92.33))
