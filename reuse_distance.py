# Computes reuse distances between two memory locations given
# loop information.

def contains(lookup_idxs, loop_idx):
    for lookup_idx in lookup_idxs:
        if lookup_idx == loop_idx:
            return True
    return False

def iteration_distance(lookup, loops):
    # Returns the number of iterations this loop value is used for.
    # For example, in the following loop:
    #
    # for i in I:
    #   for j in J:
    #       A[i]
    #
    # The iteration distance for the lookup A[i] is J, which is the number of
    # iterations in the inner loop. 
    # loops: a list of tuples in nest order. Each tuple is:
    #   (iterations, indexVar)
    # For now we'll assume iterations are known statically.
    lookup_idxs = lookup.index
    iterations = 1
    for loop in reversed(loops):
        iters, loop_idx = loop
        if contains(lookup_idxs, loop_idx):
            break
        iterations *= iters
    return iterations

def reuse_distance(lookup, loops):
    # Returns the number of iterations this loop value is used for.
    # For example, in the following loop:
    #
    # for i in I:
    #   for j in J:
    #       A[j]
    #
    # The reuse distance for the lookup A[j] is J, which is the number of
    # iterations in the inner loop. 
    # loops: a list of tuples in nest order. Each tuple is:
    #   (iterations, indexVar)
    # For now we'll assume iterations are known statically.
    # TODO this is currently very naive and needs some refinement.
    # We also need to take into account the size of each access,
    # the block sizes of the caches, etc. This may be done at a higher
    # level.
    lookup_idxs = lookup.index
    dist = 1
    for loop in reversed(loops):
        iters, loop_idx = loop
        if not contains(lookup_idxs, loop_idx):
            break
        dist *= iters
    return dist
