# Computes reuse distances between two memory locations given
# loop information.

def contains(node, f):
    if f == node:
        return True
    for c in node.children():
        if contains(c, f):
            return True
    return False

def iteration_distance(l1, loops):
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
    l1_idx = l1.index
    iterations = 1
    for loop in reversed(loops):
        iters, idx = loop
        if contains(l1_idx, idx):
            break
        iterations *= iters
    return iterations

def reuse_distance(l1, loops):
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
    l1_idx = l1.index
    dist = 1
    for loop in reversed(loops):
        iters, idx = loop
        if not contains(l1_idx, idx):
            break
        dist *= iters
    return dist
