"""
Tests costs of the following simple query:

for(V0...Vn+1):
    if V0[i] > X:
        V1[i] + V2[i] + ... + Vn[i] + 0
    else:
        0
"""

from expressions import *
from cost import *

block_sizes, latencies = [64, 64, 64], [1, 7, 45, 100]

# Number of lookups
num_lookups = 4
# Number of iterations of the for loop
iterations = 1000
# Probability of the branch being taken (1-selectivity)
sel = 0.75

def get_summed_lookups(n):
    """
    Returns Lookup(V1) + Lookup(V2) + ... Lookup(Vn)
    """
    if n == 0:
        return Literal()
    return Add(Lookup(str(n)), get_summed_lookups(n-1))

def print_result(name, value):
    print "{0}: {1}".format(name, value)


"""
The Branched Loop:
        for(V0...Vn+1):
            if V0[i] > X:
                V1[i] + V2[i] + ... + Vn[i] + 0
            else:
                0

The Scalar Predicatd Loop:
        for(V0...Vn+1):
            (V0[i] > X) * (V1[i] + V2[i] + ... + Vn[i] + 0)
"""

for v in xrange(1, 6):
    print "----- {0} -----".format(v)
    for s in [0.01, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0]:
        print "vs={0}, sel={1}".format(v, s)
        lookups = get_summed_lookups(v)
        condition = GreaterThan(Lookup("0"), Literal())
        branch_expr = If(condition, lookups, Literal())
        branch_expr.selectivity = s

        branched_loop = For(iterations, 1, branch_expr)
        predicated_expr = For(iterations, 1, Multiply(condition, lookups))

        # Compute and Print Costs
        c = cost(branched_loop, block_sizes, latencies)
        print_result("Branched", c)
        c =  cost(predicated_expr, block_sizes, latencies)
        print_result("No Branch", c)
