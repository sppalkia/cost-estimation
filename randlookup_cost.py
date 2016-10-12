from expressions import *
from cost_with_bandwidth import *

block_size, latencies = 64, [1, 7, 45, 100]

def print_result(name, value):
    print "{0}: {1}".format(name, value)

n = 25000000
for k in [100, 1000, 10000, 100000, 1000000, 10000000, 100000000]:
  print "----- {0} -----".format(k)
  print "k={0}, n={1}".format(k, n)
  loop_body = Add(Lookup(Vector("R", k), Lookup("A", Id("i"))), Literal())
  loop = For(n, Id("i"), 1, loop_body)

  c = cost(loop, block_size, latencies)
  print_result("Result", c)
