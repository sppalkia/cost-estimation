"""
Tests costs of the following loops:

for i in range(N):
    for j in range(N):
        for k in xrange(N):
            C[i][j] += (A[i][k] * B[k][j])

and

for kk in range(N, stride=blockSize):
    for jj in range(N, stride=blockSize):
        for i in range(N):
            for j in range(jj, min(kk+blockSize), N):
                for k in range(kk, min(kk+blockSize), N):
                    C[i][j] += (A[i][k] * B[k][j])

The cost of the blocked version should be less than the unblocked
version, for sufficiently large N.

"""

from expressions import *
from cost_with_bandwidth import *

block_size, latencies = 64, [1, 7, 45, 100]

def print_result(name, value):
    print "{0}: {1}".format(name, value)

for b in [128]:
  print "----- {0} -----".format(b)
  for n in [128, 256, 512, 1024, 2048]:
    print "n={0}, block_size={1}".format(n, b)

    # Loop 0 (transposed).
    k_loop = For(n, Id("k"), 1, Add(Lookup("C", [Id("i"), Id("j")]),
                                    Multiply(Lookup("A", [Id("i"), Id("k")]),
                                             Lookup("B", [Id("j"), Id("k")]))))
    j_loop = For(n, Id("j"), 1, k_loop)
    i_loop = For(n, Id("i"), 1, j_loop)
    c = cost(i_loop, block_size, latencies)
    print_result("Transposed", c)

    # Loop 1 (unblocked).
    k_loop = For(n, Id("k"), 1, Add(Lookup("C", [Id("i"), Id("j")]),
                                    Multiply(Lookup("A", [Id("i"), Id("k")]),
                                             Lookup("B", [Id("k"), Id("j")]))))
    j_loop = For(n, Id("j"), 1, k_loop)
    i_loop = For(n, Id("i"), 1, j_loop)
    c = cost(i_loop, block_size, latencies)
    print_result("Unblocked", c)

    # Loop 2 (blocked).
    k_loop = For(b, Id("k"), 1, Add(Lookup("C", [Id("i"), Id("j")]),
                                    Multiply(Lookup("A", [Id("i"), Id("k")]),
                                             Lookup("B", [Id("k"), Id("j")]))))
    j_loop = For(b, Id("j"), 1, k_loop)
    i_loop = For(n, Id("i"), 1, j_loop)
    jj_loop = For(n, Id("jj"), b, i_loop)
    kk_loop = For(n, Id("kk"), b, jj_loop)
    c = cost(kk_loop, block_size, latencies)
    print_result("Blocked", c)
