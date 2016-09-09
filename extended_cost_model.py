# Routines implementing the extended cost model.

def P_cache_miss(s, block_size):
    # Returns probability of a cache miss for a given block size.
    return 1 - pow(1 - s, block_size)

def P_seq_cache_miss(s, block_size):
    # Returns probability of a cache miss which is sequential.
    p_miss = P_cache_miss(s, block_size)
    return pow(p_miss, 2)

def P_rnd_cache_miss(s, block_size):
    # Returns the probability of a cache miss which is random.
    t = pow(1 - s, block_size)
    return t - pow(t, 2)

def num_cache_misses(width, length, block_size, prob):
    return prob * (width * length) / block_size

def llc_sequential_cost(misses, latencies):
    # Returns the cost of sequential misses in the LLC, taking
    # into account a standard adjacent cache line prefetcher.
    # misses is a tuple (Ms, Mr) of sequential and random misses.
    #
    # latencies is a list of latencies.
    # len(latencies) must == len(misses). index i = ith level of cache.
    llc = misses[-1][0] * latencies[-1]
    c = 0.0
    for i in xrange(len(misses) - 1):
        c += latencies[i] * sum(misses[i]) 
    return max(0.0, llc - c)

def memory_cost(misses, latencies):
    # Returns the total memory access cost given an estimated number 
    # of misses and latencies for a memory heirarchy.
    c = 0.0
    for i in xrange(len(misses) - 1):
        c += latencies[i+1] * sum(misses[i]) 
    return c + llc_sequential_cost(misses, latencies)

def memory_cost_for_heirarchy(s, dwidth, dlength, block_sizes, latencies):
    # Given a selectivity, a data item width, a vector length, and a list of
    # block sizes and access latencies, returns the cost of accessing the data
    # element conditionally.
    levels = len(block_sizes)
    misses = []
    for l in xrange(levels):
        p_seq = P_seq_cache_miss(s, block_size)
        p_rnd = P_rnd_cache_miss(s, block_size)
        num_seq_misses = num_cache_misses(dwidth, dlength, block_size, p_seq)
        num_rnd_misses = num_cache_misses(dwidth, dlength, block_size, p_seq)
        misses.append((num_seq_misses, num_rnd_misses))
    return memory_cost(misses, latencies)
