# Routines to get various cost metrics from an expression.
"""
The cost framework works as follows.

Costs are only computed for parallel constructs in the language, namely For
loops. Expressions not nested under for loops are not considered (this may
change later, e.g. if an "if" statement branches into two different
for loops).

Costs are decomposed into two components: a _memory cost_ and a
_processing cost_.

The processing cost states the amount of CPU used in a given loop. For example,
in the loop for(V, |x| -> x + 1), the total processing cost would be len(V),
since len(V) iterations of the loop are run and each iteration performs
(roughly) one cycle of processing. The processing cost assumes value are already
in registers.

Processing cost can also capture instruction-level optimizations such as
vectorization. For example, if we were to vectorize the above loop to have
stride 4 (for(V, 4, |x| -> x + (1,1,1,1))), the cost would become len(V) / 4
since each instruction still only takes one cycle, but 1/4 the number of iterations
are run.

Memory cost captures the cost of loading values from the memory heirarchy. The 
primary aspect captured in the memory model is prefetching. The model computes,
based on selectivity, the cost of moving data into the L1 cache and overlaps
costs of fetching data into LLC (and eventually, L2) with the processing cost
to capture adjacent cache line prefetchers of modern CPUs.

TODO : Multi-core.
"""

from expressions import *
from extended_cost_model import *

def cost(expr, block_sizes, latencies):
    # Return the cost of an expression.

    # Only consider costs of loops.
    if not isinstance(expr, For):
        return 0

    # Get the processing cost - this is the cost of running each iteration
    # N times, where N is the number of iterations in the loop. This ignores
    # costs related to the memory heirarchy and assumes all values are loaded
    # into registers.
    p_cost = processing_cost(expr)

    def _get_lookups(expr, lookups=set()):
        # Find Lookup (i.e. memory access) nodes in the expression tree.
        for c in expr.children():
            # TODO need to associate each lookup with a access probability.
            # TODO how what kind of lookup is it? Is it sequential? What's
            # the stride? If the selectivity is non-zero and the stride
            # isn't zero, how do we handle this?
            if isinstance(c, Lookup):
                lookups.add(c)
            _get_lookups(c, lookups)
        return lookups


    # The list of lookups.
    lookups = list(_get_lookups(expr))
    # The cost for a given lookup for the L1 and L2 cache.
    lookup_costs = []
    # The cost for a give lookup for the L3 cache.
    l3_seq_costs = []

    for l in lookups:
        # TODO strides
        # TODO selectivities
        # Strides can be:
        #   Constant -- access is sequential?
        #   Random (Unknown/Dynamic -- all access is random)
        for block_size, latency in zip(block_sizes[:len(block_sizes)],\
                latencies[:len(latencies)]):
            seq_cost = sequential_cost(1.0, 4 * expr.stride,
                expr.iters / expr.stride, block_size, latency)
            rnd_cost = random_cost(1.0, 4 * expr.stride,
                expr.iters / expr.stride, block_size, latency)
            lookup_costs.append(seq_cost + rnd_cost)

        l3_seq_cost = sequential_cost(1.0, 4 * expr.stride,\
                expr.iters / expr.stride, block_sizes[-1], latencies[-1])
        lookup_costs.append(l3_seq_cost)
        l3_rnd_cost = random_cost(1.0, 4 * expr.stride,\
                expr.iters / expr.stride, block_sizes[-1], latencies[-2])
        l3_seq_costs.append(l3_rnd_cost)

    print lookup_costs
    print max(l3_seq_costs)
    print p_cost

    # Get the highest L3 sequential cost. We assume all the prefetched L3 lines
    # are prefetched in parallel.
    highest_l3_cost = max(l3_seq_costs)
    # Sum the L1 and L2 lookup costs.
    mem_cost = sum(lookup_costs)

    # Get the effective L3 cost. Here, we check if the L3 cost
    # is masked by the prefetcher by "overlapping" it with the L1/L2
    # load costs and the processing cost.
    l3_cost = max(0.0, highest_l3_cost - (p_cost + mem_cost))

    # Return the sum of all the costs.
    return l3_cost + p_cost + mem_cost

def processing_cost(expr):
    # Get the processing cost of an exprssion.
    ctx = {}
    ctx['iters'] = float(expr.iters / expr.stride)
    # Return the processing cost of the For loop's expression.
    return expr.expr.cost(ctx)
