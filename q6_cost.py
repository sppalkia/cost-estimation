"""
Tests costs of the following loops:

for(V0...Vn+1):
    if V0[i] > X:
        V1[i] + V2[i] + ... + Vn[i] + 0
    else:
        0

and

for(V0...Vn+1):
    (V0[i] > X) * (V1[i] + V2[i] + ... + Vn[i] + 0)

The cost of the second loop in general should be less than the
cost of the first loop for "less-predictable" branches. For
"more-predictable" branches (i.e., branches that almost always
evaluate to true or false), the first loop should have lower cost
"""

from expressions import *
from cost_with_bandwidth import *

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
    return Add(Lookup(str(n), Id("i")), get_summed_lookups(n-1))

def print_result(name, value):
    print "{0}: {1}".format(name, value)

for v in [1, 5]:
    print "----- {0} -----".format(v)
    for s in [0.01, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0]:
        print "vs={0}, sel={1}".format(v, s)
        lookups = get_summed_lookups(v)
        condition = GreaterThan(Lookup("0", Id("i")), Literal())
        branch_expr = If(condition, lookups, Literal())
        branch_expr.selectivity = s

        branched_loop = For(iterations, Id("i"), 1, branch_expr)
        predicated_expr = For(iterations, Id("i"), 1, Multiply(condition, lookups))

        # Compute and Print Costs
        c = cost(branched_loop, block_sizes, latencies)
        print_result("Branched", c)
        c =  cost(predicated_expr, block_sizes, latencies)
        print_result("No Branch", c)
