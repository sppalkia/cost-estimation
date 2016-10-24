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

# Number of lookups
num_lookups = 4
# Number of iterations of the for loop
iterations = 2.5e7
# Probability of the branch being taken (1-selectivity)

def get_summed_lookups(n, vec=False):
    """
    Returns Lookup(V1) + Lookup(V2) + ... Lookup(Vn)
    """
    if n == 0:
        return Literal()

    vecSize = 1 if not vec else 8
    return Add(Lookup(str(n), Id("i")), get_summed_lookups(n-1, vec), vecSize)

def print_result(name, value):
    print "{0}: {1}".format(name, value)

def print_result_parsable(name, value, vs, p, iterations):
    s = ">>> "
    s += str(name[0])
    s += ":"
    s += str(value)
    s += "\t"
    s += str(iterations)
    s += "\t"
    s += str(vs)
    s += "\t"
    s += str(p)
    print s

for v in [1, 10]:
    print "----- {0} -----".format(v)
    for s in [0.01, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0]:
        print "vs={0}, sel={1}".format(v, s)
        lookups = get_summed_lookups(v)
        condition = GreaterThan(Lookup("0", Id("i")), Literal())
        branch_expr = If(condition, lookups, Literal())
        branch_expr.selectivity = s
        branched_loop = For(iterations, Id("i"), 1, branch_expr)
        predicated_expr = For(iterations, Id("i"), 1, Multiply(condition, lookups))


        vec_lookups = get_summed_lookups(v, 8)
        vec_condition = GreaterThan(Lookup("0", Id("i")), Literal(), 8)
        vector_expr = For(iterations, Id("i"), 8, Multiply(vec_condition, vec_lookups))

        # Compute and Print Costs
        c = cost(branched_loop)
        print_result("Branched", c)
        print_result_parsable("Branched", c, v, s, iterations)
        c =  cost(predicated_expr)
        print_result("No Branch", c)
        print_result_parsable("NoBranch", c, v, s, iterations)
        c =  cost(vector_expr)
        print_result("Vector", c)
        print_result_parsable("Vector", c, v, s, iterations)
