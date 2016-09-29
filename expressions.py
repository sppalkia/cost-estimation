# Implements dummy expression objects to test cost model logic on.

import math

from reuse_distance import *

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

class Id(Expr):
    def __init__(self, name):
        self.name = name

    def cost(self, ctx):
        return 0

    def __str__(self):
        return self.name

    def __eq__(self, other):
        if not isinstance(other, Id):
            return False
        return self.name == other.name

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
        return lhsCost + rhsCost + 1.

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
class Mod(BinaryExpr):
    def __str__(self):
        return str(self.left) + "%" + str(self.right)

class For(Expr):
    # A for loop.
    def __init__(self, iters, loopIdx, stride, expr):
        # The range of this loop. An 'iters' value of 1000 with a stride of 8
        # means the loop body will be executed 1000 / 8 = 125 times.
        self.iters = iters
        # Stride (designates vectorization).
        self.stride = stride
        # The loop index variable (must be an Id)
        self.loopIdx = loopIdx
        # The expression representing the body of this loop.
        self.expr = expr

        # Some simple type checking to prevent Pythonic headaches.
        assert isinstance(self.loopIdx, Id)
        assert isinstance(self.expr, Expr)

    def children(self):
        return [self.expr]

    def __str__(self):
        return "for({0},{1},{2},{3})".format(str(self.iters),
                str(self.loopIdx),
                str(self.stride),
                str(self.expr))

    def cost(self, ctx):
        if "loops" not in ctx:
            ctx["loops"] = []

        # Keep a stack of loops so costs can be derived based on loop nesting.
        iterations = float(self.iters) / self.stride
        ctx["loops"].append((iterations, self.loopIdx))
        exprCost = self.expr.cost(ctx)
        ctx["loops"].pop()
        return exprCost * iterations


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

        # TODO Simplistic, but we assume the condition depends on lookups
        # and constants only. We can thus determine a branch prediction cost
        # depending on whether the same "result" from the branch condition is
        # used multiple times.
        ids = []
        stack = [self.cond]
        while len(stack) != 0:
            node = stack.pop()
            for c in node.children():
                stack.append(c)
            if isinstance(node, Lookup):
                ids.append(node)

        # Get the smallest iteration distance. If it's sufficiently small, we
        # use it to amortize the branch prediction cost.
        it_distance = min([iteration_distance(id_node, ctx["loops"]) for id_node in ids])

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
        branch_penalty = -1. * pow(self.selectivity * 2. - 1., 2.) + 1.

        # Picked this arbitrarily...
        if it_distance > 10:
            branch_penalty = 0.0
        return condCost + p_true * trueCost + p_false * falseCost +\
                branch_penalty * 5.0

    def __str__(self):
        return "if({0},{1},{2})".format(str(self.cond),
                str(self.true),
                str(self.false))

class Vector(Expr):
    def __init__(self, name, length):
        self.name = name
        self.length = length

    def cost(self, ctx):
        # TODO
        return 0

    def __str__(self):
        return "vec({0},{1})".format(self.name, self.length)

class Lookup(Expr):
    # A memory lookup into an array.
    def __init__(self, vector, index):

        if isinstance(vector, str):
            self.vector = Vector(vector, 0)
        else:
            self.vector = vector

        if not isinstance(index, list):
            self.index = [index]
        else:
            self.index = index
        self.sequential = False

    def __eq__(self, other):
            return isinstance(other, Lookup) and\
                    self.vector == other.vector and\
                    self.index == other.index

    def __hash__(self):
        return hash(str(self.vector) + str(self.index))

    def __str__(self):
        return "lookup({0},{1})".format(str(self.vector),
                str(self.index) if self.index is not None else "i")

    def cost(self, ctx):
        # Sets annotations on a lookup node; doesn't perform any cost calculation.

        def is_sequential(lookup, indices):
            # given a lookup expression and an ordered list of loop
            # indices (ordered by nesting), returns whether an access pattern
            # is sequential.
            #
            # Caveat: Only consider simple arithmetic computations right now
            # (i.e., only Add, Subtract, Multiply and Divide Expr nodes are allowed in the
            # index computation).
            #
            # TODO this should later be augmented to return the "distance" of
            # reuse between two accesses (e.g. A[j][i] has a reuse distance of len(j))

            def contains(node, f):
                if f == node:
                    return True
                for c in node.children():
                    if contains(c, f):
                        return True
                return False

            # None lookup nodes designate implicit sequential access.
            if lookup is None:
                return True
            index = lookup.index[-1]

            # If the last loop index is accessed non-sequentially, the access is
            # not sequential.
            if isinstance(index, Multiply) or isinstance(index, Divide):
                if contains(index, indices[-1]):
                    return False

            # Don't support complex expressions for now.
            # TODO this needs to be slightly more complex (e.g. Mods are special).
            if isinstance(index, BinaryExpr):
                if isinstance(index, Add) or isinstance(index, Subtract):
                    for c in index.children():
                        if not is_sequential(c, indicies):
                            return False
                    return True

            # This is the "base case"
            if isinstance(index, Id):
                return True

            return False

        # Memory access costs determined separately.
        if "selectivity" in ctx:
            self.p_execute = ctx["selectivity"]
        else:
            self.p_execute = 1.0
        self.loops_seq = []
        for loop in ctx["loops"]:
            self.loops_seq.append(loop)

        iters = [loop[0] for loop in ctx["loops"]]
        self.loops = reduce(lambda x,y: x*y, iters)

        idxes = [loop[1] for loop in ctx["loops"]]
        self.sequential = is_sequential(self, idxes)

        return 0.
