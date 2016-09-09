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

The multicore cost and single core cost may be the same as long as everything is
"embarassingly parallel". Namely, the total work done remains constant, and
"""

from expressions import *
from extended_cost_model import *

def cost(expr, block_sizes, latencies):
    # Return the cost of an expression.
    if not isinstance(expr, For):
        return 0
    p_cost = processing_cost(expr)

    def _get_lookups(expr, lookups=set()):
        for c in expr.children():
            # TODO need to associate each lookup with a access probability.
            # TODO how what kind of lookup is it? Is it sequential? What's
            # the stride? If the selectivity is non-zero and the stride
            # isn't zero, how do we handle this?
            if isinstance(c, Lookup):
                lookups.add(c)
            _get_lookups(c, lookups)
        return lookups

    lookups = list(_get_lookups(expr))
    lookup_costs = []
    l3_seq_costs = []
    for l in lookups:
        # TODO will need selectivities and strides here too.
        # Strides can be:
        #   Constant -- access is sequential?
        #   Random (Unknown/Dynamic -- all access is random)
        l1_cost = memory_cost(1.0, 4, expr.iters / expr.stride, block_sizes[0], latencies[0]) 
        l2_cost = memory_cost(1.0, 4, expr.iters / expr.stride, block_sizes[1], latencies[1]) 
        lookup_costs.append(l1_cost + l2_cost)
        l3_seq_cost = llc_sequential_cost(1.0, 4, expr.iters / expr.stride, block_sizes[2], latencies[2])
        l3_rnd_cost = llc_random_cost(1.0, 4, expr.iters / expr.stride, block_sizes[2], latencies[3])
        lookup_costs.append(l3_rnd_cost)
        l3_seq_costs.append(l3_seq_cost)

    # Get the highest L3 cost
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
    ctx = {}
    ctx['iters'] = expr.iters
    # Return the processing cost of the For loop's expression.
    return expr.expr.cost(ctx)
