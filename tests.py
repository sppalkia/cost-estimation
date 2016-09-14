# Simple tests to ensure costs are "as expected"

from expressions import *
from cost import *

block_sizes, latencies = [8, 64, 64], [1, 3, 8, 12]

# Number of lookups
num_lookups = 1
# Number of iterations of the for loop
iterations = 1000

def get_summed_lookups(n):
    """
    Returns Lookup(V1) + Lookup(V2) + ... Lookup(Vn)
    """
    if n == 0:
        return Literal()
    return Add(Lookup(str(n)), get_summed_lookups(n-1))


lookups = get_summed_lookups(num_lookups)
condition = GreaterThan(Lookup("0"), Literal())
branch_expr = If(condition, lookups, Literal())

"""
for(V0...Vn+1):
    if V0[i] > X:
        V1[i] + V2[i] + ... + Vn[i] + 0
    else:
        0
"""
branched_loop = For(iterations, 1, branch_expr)

"""
for(V0...Vn+1):
    (V0[i] > X) * (V1[i] + V2[i] + ... + Vn[i] + 0)
"""
predicated_expr = For(iterations, 1, Multiply(condition, lookups))

"""
for(V0...Vn+1):
    (V0[i..i+7] > X) * (V1[i..i+7] + V2[i..i+7] + ... + Vn[i..i+7] + 0)
"""
predicated_vectorized_expr = For(iterations, 8, Multiply(condition, lookups))

print branched_loop
c = cost(branched_loop, block_sizes, latencies)
print "Branched: {0}".format(c)

print predicated_expr
c =  cost(predicated_expr, block_sizes, latencies)
print "Pred: {0}".format(c)

print predicated_vectorized_expr
c =  cost(predicated_vectorized_expr, block_sizes, latencies)
print "PredVec: {0}".format(c)
