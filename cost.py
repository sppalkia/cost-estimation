# Routines to get various cost metrics from an expression.
def cost(expr, block_sizes, latencies):
    # Return the cost of an expression.
    if not isinstance(expr, For):
        return 0
    p_cost = processing_cost(expr)
    lookups = set()
    for c in expr.children():
        # TODO need to associate each lookup with a access probability.
        # TODO how what kind of lookup is it? Is it sequential? What's
        # the stride? If the selectivity is non-zero and the stride
        # isn't zero, how do we handle this?
        if isinstance(c, Lookup):
            lookups.add(c)
    lookups = list(lookups)
    lookup_costs = []
    l3_seq_costs = []
    for l in lookups:
        # TODO will need selectivities and strides here too.
        # Strides can be:
        #   Constant -- access is sequential?
        #   Random (Unknown/Dynamic -- all access is random)
        l1_cost = memory_cost(block_sizes[0], latencies[0]) 
        l2_cost = memory_cost(block_sizes[1], latencies[1]) 
        lookup_costs.append(l1_cost + l2_cost)
        l3_seq_cost = llc_sequential_cost(block_sizes[2], latencies[2])
        l3_seq_costs.append(l3_seq_cost)

    # Get the highest L3 cost
    highest_l3_cost = max(l3_seq_cost)
    # Sum the L1 and L2 lookup costs.
    mem_cost = sum(lookup_costs)

    # Get the effective L3 cost. Here, we check if the L3 cost
    # is masked by the prefetcher by "overlapping" it with the L1/L2
    # load costs and the processing cost.
    l3_cost = max(0.0, highest_l3_cost - (p_cost + mem_cost))0

    # Return the sum of all the costs.
    return l3_cost + p_cost + mem_cost

def processing_cost(expr):
    ctx = {}
    ctx['iters'] = expr.iters
    # Return the processing cost of the For loop's expression.
    return expr.expr.cost(ctx)
