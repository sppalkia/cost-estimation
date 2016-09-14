# Implements dummy expression objects to test cost model logic on.

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

# Literals have no cost.
class Literal(Expr):
    def cost(self, ctx):
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
    def __init__(self, cond, true, false):
        self.cond = cond
        self.true = true
        self.false = false

    def children(self):
        return [self.cond, self.true, self.false]

    def cost(self, ctx):
        # TODO: Some cost to branching, perhaps to model prediction.
        # Right now, this assumes a selectivity of 1.
        return self.cond.cost(ctx) +self.true.cost(ctx)

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
        return 0.

