# Implements dummy expression objects to test cost model logic on.

import math

class Expr:
    # Root expression class
    newId = 0
    @property
    def id(self):
        if self._id == None:
            self._id = Expr.newId
            Expr.newId += 1
        return self._id

    def cost(self, ctx):
        raise NotImplementedError

    def children(self):
        return []

    def p_execute(self):
        """
        The probability that this node will be executed.
        """
        return 1.0

# Literals have no cost.
class Literal(Expr):
    def cost(self, ctx):
        if "selectivity" in ctx:
            self.p_execute = ctx["selectivity"]
        else:
            self.p_execute = 1.0

        return 0.

    def __str__(self):
        return "X"

class BinaryExpr(Expr):
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def children(self):
        return [self.left, self.right]

    def cost(self, ctx):
        """
        Cost components:
        - LHS expression cost.
        - RHS expression cost.
        - 1 (to perform the comparison)
        """
        if "selectivity" in ctx:
            self.p_execute = ctx["selectivity"]
        else:
            self.p_execute = 1.0

        lhsCost = self.left.cost(ctx)
        rhsCost = self.right.cost(ctx)
        return lhsCost + rhsCost + 1. * ctx['iters']

# Basic Binary expressions, whose cost is computed as being
# the costs of the LHS and RHS expressions + 1 for the
# actual instruction.
class GreaterThan(BinaryExpr):
    def __str__(self):
        return str(self.left) + ">" + str(self.right)
class LogicalAnd(BinaryExpr):
    def __str__(self):
        return str(self.left) + "&&" + str(self.right)
class BitwiseAnd(BinaryExpr):
    def __str__(self):
        return str(self.left) + "&" + str(self.right)
class Add(BinaryExpr):
    def __str__(self):
        return str(self.left) + "+" + str(self.right)
class Subtract(BinaryExpr):
    def __str__(self):
        return str(self.left) + "-" + str(self.right)
class Multiply(BinaryExpr):
    def __str__(self):
        return str(self.left) + "*" + str(self.right)
class Divide(BinaryExpr):
    def __str__(self):
        return str(self.left) + "/" + str(self.right)

class For(Expr):
    # A for loop.
    def __init__(self, iters, stride, expr):
        self.iters = iters
        self.stride = stride
        self.expr = expr

    def children(self):
        return [self.expr]

    def __str__(self):
        return "for({0},{1},{2})".format(str(self.iters),
                str(self.stride),
                str(self.expr))

class If(Expr):
    # A conditional branch.
    # Possible annotations:
    def __init__(self, cond, true, false):
        self.cond = cond
        self.true = true
        self.false = false

        # Annotations are just fields in the object.
        self.selectivity = 1.0

    def children(self):
        return [self.cond, self.true, self.false]

    def cost(self, ctx):
        condCost = self.cond.cost(ctx)
        if "selectivity" in ctx:
            old_s = ctx["selectivity"]
            ctx["selectivity"] *= self.selectivity
        else: 
            old_s = 1.0
            ctx["selectivity"] = self.selectivity

        p_true = ctx["selectivity"]
        trueCost = self.true.cost(ctx)

        p_false = old_s * (1 - self.selectivity)
        ctx["selectivity"] = p_false
        falseCost = self.false.cost(ctx)

        # Restore the selectivity; this effectively "pops" downstream changes.
        ctx["selectivity"] = old_s
        self.p_execute = ctx["selectivity"]

        # A penalty for branch mispredicts. Closer to 0 (no penalty) when
        # selectivities are predictable, i.e. closer to 0 or 1.
        branch_penalty = -1 * pow(self.selectivity * 2 - 1, 2) + 1
        return condCost + p_true * trueCost + p_false * self.false.cost(ctx) +\
                branch_penalty * ctx["iters"] * 5

    def __str__(self):
        return "if({0},{1},{2})".format(str(self.cond),
                str(self.true),
                str(self.false))

class Lookup(Expr):
    # A memory lookup into an array.
    def __init__(self, vector, index=None):
        self.vector = vector
        # None index means sequential stride in a loop.
        self.index = index

    def __eq__(self, other):
            return isinstance(other, Lookup) and\
                    self.vector == other.vector and\
                    self.index == other.index

    def __hash__(self):
        return hash(self.vector + str(self.index))

    def __str__(self):
        return "lookup({0},{1})".format(str(self.vector),
                str(self.index) if self.index is not None else "i")

    def cost(self, ctx):
        # Memory access costs determined separately.
        if "selectivity" in ctx:
            self.p_execute = ctx["selectivity"]
        else:
            self.p_execute = 1.0

        return 0.
