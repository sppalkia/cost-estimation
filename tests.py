# Simple tests to ensure costs are "as expected"

from expressions import *
from cost import *

vectorA = "A"

bin_expr = Add(Lookup(vectorA), Literal())
# A simple for loop with 1000 elements, strided sequentially.
simple_for = For(1000, 1, bin_expr)
print cost(simple_for, [8, 64, 64], [1, 3, 8, 12])

more_bin_exprs = Add(bin_expr, Literal())
another_for = For(1000, 1, more_bin_exprs)
print cost(another_for, [8, 64, 64], [1, 3, 8, 12])
