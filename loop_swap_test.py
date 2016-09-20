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

# Number of iterations of the for loop
iterations = 1000

def print_result(name, value):
    print "{0}: {1}".format(name, value)

inner_id = Id("j")
predicate = If(GreaterThan(Lookup("A", inner_id), Literal()), Add(Literal(), Literal()), Literal())
# The predicate is data dependent and unpredictable.
predicate.selectivity = 0.5
inner_loop = For(iterations, inner_id, 1, predicate)
outer_loop = For(iterations, Id("i"), 1, inner_loop)


# Interchange the loops.
inner_loop_2 = For(iterations, Id("i"), 1, predicate)
outer_loop_2 = For(iterations, inner_id, 1, inner_loop_2)

print outer_loop
print outer_loop_2

print cost(outer_loop, block_sizes, latencies)
print cost(outer_loop_2, block_sizes, latencies)
