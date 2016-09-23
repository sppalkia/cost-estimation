"""
Tests costs of the following loops:

for i in range(N):
    for j in range(M):
        if A[j] > X:
            sum += A[j]

and

for j in range(M):
    for i in range(N):
        if A[j] > X:
            sum += A[j]

The cost of the two queries should be equal.

"""

from expressions import *
from cost import *

block_sizes, latencies = [64, 64, 64], [1, 7, 45, 100]

def print_result(name, value):
    print "{0}: {1}".format(name, value)

for k in [10, 100, 1000]:
  print "----- {0} -----".format(k)
  for n in [10000, 20000, 30000, 40000, 50000, 100000]:
    print "k={0}, n={1}".format(k, n)

    # Predicate definition. Always use index j for array lookups in the predicate.
    predicate = If(GreaterThan(Lookup("A", Id("j")), Literal()), Add(Literal(), Literal()), Literal())
    predicate.selectivity = 0.5

    # The predicate is data dependent and unpredictable.
    inner_loop = For(n, Id("j"), 1, predicate)
    outer_loop = For(k, Id("i"), 1, inner_loop)
    c = cost(outer_loop, block_sizes, latencies)
    print_result("Unblocked", c)

    # Interchange the loops.
    inner_loop_2 = For(k, Id("i"), 1, predicate)
    outer_loop_2 = For(n, Id("j"), 1, inner_loop_2)
    c = cost(outer_loop_2, block_sizes, latencies)
    print_result("Interchanged", c)
