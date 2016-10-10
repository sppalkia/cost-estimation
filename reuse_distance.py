# Computes reuse distances between two memory locations given
# loop information.

# TODO get this from the AST.
ELEM_SIZE = 4.0

def contains(lookup_idxs, loop_idx):
    # Determines if any of the lookup indices (specified by lookup_idxs)
    # are the same as the passed-in loop index
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
    # For now we'll assume loop bounds are known statically.
    lookup_idxs = lookup.index
    iterations = 1
    for loop in reversed(loops):
        iters, loop_idx = loop
        if contains(lookup_idxs, loop_idx):
            break
        iterations *= iters
    return iterations

def get_is_sequential(seen_loops, lookup_idxs):
    is_sequential = False
    for seen_loop in seen_loops:
        _, seen_loop_idx = seen_loop
        if seen_loop_idx == lookup_idxs[-1]:
            is_sequential = True
        for lookup_idx in lookup_idxs:
            if lookup_idx == seen_loop_idx:
                return is_sequential
    return is_sequential

def reuse_distance_to_self(lookup_idxs, loops, should_break=True):
    # Given a set of lookup ids and a sequence of loops, determines the number
    # of distinct blocks accessed in the passed-in array until the same element
    # is reached again.
    # Also, returns a list of loop indices while traversing through the loops
    # array.
    dist = 1
    seen_loops = list()
    for loop in loops:
        iters, loop_idx = loop
        if should_break:
            if not contains(lookup_idxs, loop_idx):
                break
            seen_loops.append(loop)
            dist *= iters
        else:
            if contains(lookup_idxs, loop_idx):
                dist *= iters
            seen_loops.append(loop)

    is_sequential = get_is_sequential(seen_loops, lookup_idxs)
    if is_sequential:
        dist = int(dist * ELEM_SIZE / 64) + 1   # Each element is 4 bytes (integer) and each cache block
                                                # is 64 bytes. Add 1 since int() rounds down.
    else:
        dist = int(dist)
    return dist, seen_loops

def reuse_distance_to_next(lookup_idxs, loops, should_break=True):
    # Given a set of lookup ids and a sequence of loops, determines the number
    # of distinct blocks accessed in the passed-in array until the _next_ element
    # is reached again.
    # Also, returns a list of loop indices while traversing through the loops
    # array.
    dist = 1
    seen_loops = list()
    for loop in loops:
        iters, loop_idx = loop
        if should_break:
            if contains([lookup_idxs[-1]], loop_idx):
                break
            seen_loops.append(loop)
            if contains(lookup_idxs, loop_idx):
                dist *= iters
        else:
            if contains(lookup_idxs, loop_idx):
                dist *= iters
            seen_loops.append(loop)

    is_sequential = get_is_sequential(seen_loops, lookup_idxs)
    if is_sequential:
        dist = int(dist * ELEM_SIZE / 64) + 1   # Each element is 4 bytes (integer) and each cache block
                                                # is 64 bytes. Add 1 since int() rounds down.
    else:
        dist = int(dist)
    return dist, seen_loops

def reuse_distance(lookup, other_lookups, loops):
    # Returns the number of memory accesses made before reusing the same
    # lookup.
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
    # For now we'll assume loop bounds are known statically.
    # TODO this is currently very naive and needs some refinement.
    # We also need to take into account the size of each access,
    # the block sizes of the caches, etc. This may be done at a higher
    # level.
    lookup_idxs = lookup.index
    all_other_lookup_idxs = [other_lookup.index for other_lookup in other_lookups
                             if other_lookup.index != lookup.index]

    # Determine number of accesses made to the same array before a particular
    # element is re-used.
    dist, seen_loops = reuse_distance_to_self(lookup_idxs, reversed(loops))

    # Determine number of accesses made to the other arrays before an element
    # in the original array is re-used.
    for other_lookup_idxs in all_other_lookup_idxs:
        other_dist, _ = reuse_distance_to_self(other_lookup_idxs, seen_loops,
                                               should_break=False)
        dist += other_dist

    # TODO: Make this dependent on parameter values (block sizes, ELEM_SIZE)
    dist = dist / 16.  # Divide by 16 here, since only one element in every cache line
                       # actually depends on the previous time the same element was re-used
                       # to determine reuse-distance. All other elements in the same cache
                       # line only depend on the previous element being accessed.

    dist_to_next, seen_loops = reuse_distance_to_next(lookup_idxs, reversed(loops))
    for other_lookup_idxs in all_other_lookup_idxs:
        other_dist_to_next, _ = reuse_distance_to_next(other_lookup_idxs, seen_loops,
                                                       should_break=False)
        dist_to_next += other_dist_to_next

    # 15 out of 16 elements depend on the number of distinct elements accessed since the
    # previous element in the _same_ cache line is accessed.
    # TODO: Make this dependent on parameter values (block sizes, ELEM_SIZE)
    dist += (dist_to_next * (15. / 16.))
    dist = int(dist)

    return dist
