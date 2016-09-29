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

Memory cost captures the cost of loading values from the memory heirarchy. This
is an attempt at simplifying the model used before, making sure to take into
account the bandwidth of the memory sub-system.

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
    p_cost = expr.cost({})
    clock_frequency = (2. * 10**9)
    memory_throughput = [(128. * 10**9), (64. * 10**9), (32. * 10**9), (4. * 10**9)]
    cache_sizes = [500, 4000, 62500]

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
    # Total number of lookups.
    num_lookups = 0

    m_cost = 0
    for l in lookups:
        num_lookups = (l.loops * l.p_execute)
        l_reuse_distance = reuse_distance(l, lookups, l.loops_seq)

        # TODO do we still need this?
        if not l.sequential:
            vector_size = l.vector.length * 4.0
            l_reuse_distance += (vector_size / block_sizes[0])

        if l.sequential:
            # Sequential access - use bandwidth.
            cache_level = len(cache_sizes)
            for i in xrange(len(cache_sizes)):
                if cache_sizes[i] > l_reuse_distance:
                    cache_level = i
                    break
            mem_lookups = (num_lookups * 4.0)
            m_cost += (((mem_lookups) / memory_throughput[cache_level]) * clock_frequency)
        else:
            # Random access - use latency.
            rand_cost = 0.0
            vector_size = l.vector.length * 4.0
            prev_p = 0.0
            for i in xrange(len(block_sizes)):
                # Number of blocks in the vector.
                blocks = vector_size / block_sizes[i]
                cache_size_in_blocks = cache_sizes[i]
                p = cache_size_in_blocks / blocks
                if p < 0.0:
                    p = 0.0
                if p > 1.0:
                    p = 1.0
                old_p = p
                p -= prev_p
                prev_p += p

                print i,old_p,p
                rand_cost += p * latencies[i]
                if old_p == 1.0:
                    break

            if prev_p != 1.0:
                p = 1.0 - prev_p
                rand_cost += p * latencies[-1]

        m_cost += (num_lookups * rand_cost)

    # Add memory and processing cost here.
    return m_cost

def processing_cost(expr):
    # Get the processing cost of an exprssion.
    ctx = {}
    ctx['iters'] = float(expr.iters / expr.stride)
    # Return the processing cost of the For loop's expression.
    return expr.expr.cost(ctx)
