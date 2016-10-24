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

CORES = 4


# Vectors of order records.
customer_idxs = Vector("customer_idxs", 25000)
orderdates = Vector("orderdates", 25000)
# Vectors of customer records.
mktsegments = Vector("mktsegments", 2500)

# An ID referring to the struct at the desired builder index.
builderId = Id("b")

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
    iterations = iterations / CORES
    loopVar = Id("i")

    # Vectors of line items.
    order_idxs = Vector("order_idxs", iterations)
    shipdates = Vector("shipdates", iterations)
    extendedprices = Vector("extendedprices", iterations)
    discounts = Vector("discounts", iterations)
    buckets = Vector("buckets", iterations)

    mergeExpr = StructLiteral([
            Add(Multiply(Lookup(extendedprices, loopVar), Subtract(Literal(), Lookup(discounts, loopVar))), GetField(builderId, 0)),
            ])

    # (line.4 > n)
    mergeIndex = Lookup(buckets, loopVar)
    vecMergerSize = b * CORES if not globalTable else b
    condition = GreaterThan(Id("shipdate"), Literal())
    branch = If(condition, VecMergerMerge(Vector("B", vecMergerSize), mergeIndex, mergeExpr, BUCKET_SIZE, globalTable), Literal())
    shipdateLet = Let(Id("shipdate"), Lookup(shipdates, loopVar), branch)
    mktsegmentLet = Let(Id("mktsegment"), Lookup(mktsegments, Id("customer_idx")), shipdateLet)
    orderdateLet = Let(Id("orderdate"), Lookup(orderdates, Id("order_idx")), mktsegmentLet)
    customeridxLet = Let(Id("customer_idx"), Lookup(customer_idxs, Id("order_idx")), orderdateLet)
    loopBody = Let(Id("order_idx"), Lookup(order_idxs, loopVar), customeridxLet)

    # Set the selectivity of the branch.
    branch.selectivity = p
    # Construct the loop body - use Let statements to get the structs into variables.
    expr = For(iterations, loopVar, 1, loopBody)

    c = cost(expr)
    print c

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
        result = For(b, Id("r"), 1, Add(Lookup("br", Id("r"), BUCKET_SIZE), FixedCostExpr(2)))  # TODO: Check me
        resCost = cost(result) * (CORES + 1)
        print resCost

    print_result(label, c + resCost)
    print_result_parsable(label, c + resCost, b, p, iterations * params.CORES)

for b in [10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000]:
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
        costForQuery(b, p, iterations, True)
        costForQuery(b, p, iterations, False)
