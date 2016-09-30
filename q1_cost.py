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

builder = VecMerger(mergeFunc)

# line.5 * X + line.6
mergeIndex = Add(Multiply(GetField("line", 5), Literal()), GetField("line", 6))

# The merge expression loads the value in the builder for each struct field and 
# adds it to a new value generated during this loop iteration.
mergeExpr = StructLiteral([
        Add(GetField("line", 0), GetField("b", 0)), 
        Add(GetField("line", 1), GetField("b", 1)),
        Add(GetField("line", 2), GetField("b", 2)),
        Add(Multiply(GetField("line", 1), Subtract(Literal(), GetField("line", 2))), GetField("b", 3)),
        Add(Multiply(Multiply(GetField("line", 1), Subtract(Literal(), GetField("line", 2))), Add(Literal(), GetField("line", 3))), GetField("b", 4)),
        Add(Literal(), GetField("b", 5)),
        ])

condition = GreaterThan(GetField("line", 4), Literal())
loopBody = If(condition, VecMergerMerge(builder, mergeIndex, mergeExpr), Literal()) 
loop = For(iterations, Id("i"), 1, loopBody)
