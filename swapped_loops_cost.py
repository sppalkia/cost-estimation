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

The cost of the second nested loop construct should be less
than the cost of the first nested loop construct, on account of
the more predictable branch.

"""

from expressions import *
from cost_with_bandwidth import *

def print_result(name, value):
    print "{0}: {1}".format(name, value)

for k in [10, 100, 500, 800, 1000]:
  print "----- {0} -----".format(k)
  for n in [10000]:
    print "k={0}, n={1}".format(k, n)

    # Predicate definition. Always use index j for array lookups in the predicate.
    predicate = If(GreaterThan(Lookup("A", Id("j")), Literal()), Add(Literal(), Literal()), Literal())
    predicate.selectivity = 0.5

    # The predicate is data dependent and unpredictable.
    inner_loop = For(n, Id("j"), 1, predicate)
    outer_loop = For(k, Id("i"), 1, inner_loop)
    c = cost(outer_loop)
    print_result("Original", c)

    # Interchange the loops.
    inner_loop_2 = For(k, Id("i"), 1, predicate)
    outer_loop_2 = For(n, Id("j"), 1, inner_loop_2)
    c = cost(outer_loop_2)
    print_result("Interchanged", c)
