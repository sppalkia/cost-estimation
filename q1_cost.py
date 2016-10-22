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

import params

# An ID referring to the struct at the current loop index.
lineId = Id("line")
# An ID referring to the struct at the desired builder index.
builderId = Id("b")

# line.5 * X + line.6
mergeIndex = GetField(lineId, 0)

iterations = 2.5e7

BUCKET_SIZE = 24

def print_result(name, value):
    print "{0}: {1}".format(name, value)

def print_result_parsable(name, value, b, p, iterations):
    s = ">>> "
    s += str(name[0])
    s += ":"
    s += str(value)
    s += "\t"
    s += str(iterations)
    s += "\t"
    s += str(b)
    s += "\t"
    s += str(p)
    print s

def costForQuery(b, p, iterations, globalTable):

    iterations = iterations / params.CORES

    mergeExpr = StructLiteral([
            Add(GetField(lineId, 0), GetField(builderId, 0)),
            Add(GetField(lineId, 1), GetField(builderId, 1)),
            Add(GetField(lineId, 2), GetField(builderId, 2)),
            Add(Multiply(GetField(lineId, 1), Subtract(Literal(), GetField(lineId, 2))), GetField(builderId, 3)),
            Add(Multiply(Multiply(GetField(lineId, 1), Subtract(Literal(), GetField(lineId, 2))), Add(Literal(), GetField(lineId, 3))), GetField(builderId, 4)),
            Add(Literal(), GetField(builderId, 5)),
            ])

    # (line.4 > n)
    vecMergerSize = b * params.CORES if not globalTable else b
    condition = GreaterThan(GetField(lineId, 4), Literal())
    branch = If(condition, VecMergerMerge(Vector("B", vecMergerSize), mergeIndex, mergeExpr, BUCKET_SIZE, globalTable), Literal())
    # Set the selectivity of the branch.
    branch.selectivity = p
    loopVar = Id("i")
    # Construct the loop body - use Let statements to get the structs into variables.
    loopBody = Let(lineId, Lookup("V", loopVar, BUCKET_SIZE), branch)
    expr = For(iterations, Id("i"), 1, loopBody)

    c = cost(expr)

    if globalTable:
        label = "Global"
        result = FixedCostExpr(10000)
        resCost = cost(result)
    else:
        label = "Local"
        # TODO switch this to use VecMergerResult. This for loop represents the
        # merging of the global tables. This is also slightly broken because
        # the  fixed cost of merging the results isn't 1...it's much higher
        # than that.
        resBody = Let(lineId, Lookup("V", Id("r"), BUCKET_SIZE), mergeExpr)
        result = For(b, Id("r"), 1, Add(Lookup("br", Id("r"), BUCKET_SIZE),
            resBody))
        resCost = cost(result) * (params.CORES)

    print_result(label, c + resCost)
    print_result_parsable(label, c + resCost, b, p, iterations * params.CORES)

for b in [int(x) for x in [1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8]]:
    print "----- {0} -----".format(b)
    for p in [0.01, 1.0]:
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
        costForQuery(b, p, iterations, True)
        costForQuery(b, p, iterations, False)
