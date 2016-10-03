# Estimates cost for a Q1-like query, where
# results are aggregated and grouped into a very small hash table.

# Modeling updates into a hash table.

# New AST Nodes for this application.

# Specifies the merge function.
"""

# Merges a value using mergeExpr at the given index.
# mergeExpr should be representative of the expression that will be generated
# when
VecMergerMerge(name, index, mergeExpr)
GetField(struct, index)
StructLiteral(exprs)

Things to model that are new:
    - GetFields with variable load sizes (this shouldn't be difficult)
    - GetFields and their affect on surrounding things
        (e.g., GetField(0), GetField(2))
    - Writes in VecMergers
"""

from expressions import *
from cost_with_bandwidth import cost

# An ID referring to the struct at the current loop index.
lineId = Id("line")
# An ID referring to the struct at the desired builder index.
builderId = Id("b")

# line.5 * X + line.6
mergeIndex = GetField(lineId, 0)

iterations = 1000

def print_result(name, value):
    print "{0}: {1}".format(name, value)

for b in [100, 1000, 10000, 100000, 1000000, 10000000, 100000000]:
    print "----- {0} -----".format(b)
    for p in [0.01, 0.1, 0.5, 0.75, 1.0]:
        print "n={0}, b={1}, p={2}".format(iterations, b, p)
        # The merge expression loads the value in the builder for each struct field and
        # adds it to a new value generated during this loop iteration.
        #
        # [
        #   line.0 + b.0,
        #   line.1 + b.1,
        #   line.2 + b.2,
        #   line.1 * ( n - line.2) + b.3,
        #   (line.1 * (1 - line.2)) * (1 + line.3) + b.4
        #   1 + b.5
        mergeExpr = StructLiteral([
                Add(GetField(lineId, 0), GetField(builderId, 0)),
                Add(GetField(lineId, 1), GetField(builderId, 1)),
                Add(GetField(lineId, 2), GetField(builderId, 2)),
                Add(Multiply(GetField(lineId, 1), Subtract(Literal(), GetField(lineId, 2))), GetField(builderId, 3)),
                Add(Multiply(Multiply(GetField(lineId, 1), Subtract(Literal(), GetField(lineId, 2))), Add(Literal(), GetField(lineId, 3))), GetField(builderId, 4)),
                Add(Literal(), GetField(builderId, 5)),
                ])

        # (line.4 > n)
        condition = GreaterThan(GetField(lineId, 4), Literal())
        branch = If(condition, VecMergerMerge(Vector("B", b), mergeIndex, mergeExpr, 24), Literal())
        # Set the selectivity of the branch.
        branch.selectivity = p

        loopVar = Id("i")

        # Construct the loop body - use Let statements to get the structs into variables.
        loopBody = Let(lineId, Lookup("V", loopVar), branch)
        expr = For(iterations, Id("i"), 1, loopBody)

        c = cost(expr)
        print_result("Result", c)
