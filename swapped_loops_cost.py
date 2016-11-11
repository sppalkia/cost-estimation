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

s = 0.01

for k in [10000]:
  print "----- {0} -----".format(k)
  for n in [10000]:
    print "k={0}, n={1}".format(k, n)

    # DIM = 100
    compute_i  = For(100, Id("k"), 1, FixedCostExpr(1))
    inner_expression = For(100, Id("n"), 1, Multiply(Literal(), Lookup("0", Id("n"))))

    # Predicate definition. Always use index j for array lookups in the predicate.
    predicate = If(GreaterThan(Lookup("A", Id("j")), Literal()),
            inner_expression, Literal())
    predicate.selectivity = s
    # The predicate is data dependent and unpredictable.
    inner_loop = For(n, Id("j"), 1, predicate)
    outer_loop = For(k, Id("i"), 1, Let("z", compute_i, inner_loop))
    c = (cost(outer_loop)) / 1e10
    print_result("Original", c)

    # Interchange the loops.
    inner_loop_2 = For(k, Id("i"), 1, Let("z", compute_i, inner_expression))
    predicate = If(GreaterThan(Lookup("A", Id("j")), Literal()),
            inner_loop_2, Literal())
    predicate.selectivity = s
    outer_loop_2 = For(n, Id("j"), 1, predicate)
    c = (cost(outer_loop_2)) / 1e10
    print_result("Interchanged", c)

    precomputed_expr = For(k, Id("m"), 1, compute_i);
    inner_loop_2 = For(k, Id("i"), 1, inner_expression)
    predicate = If(GreaterThan(Lookup("A", Id("j")), Literal()),
            inner_loop_2, Literal())
    predicate.selectivity = s
    outer_loop_2 = For(n, Id("j"), 1, predicate)
    c = (cost(outer_loop_2)) / 1e10
    c = (cost(outer_loop_2) + cost(precomputed_expr)) / 1e10
    print_result("Cached", c)
