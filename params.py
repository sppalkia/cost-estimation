# Machine parameters/constants for the CBO.

# ************************* System/CPU Parameters  *************************

# Number of CPU cores.
CORES = 4

# 2.9 GHz
CLOCK_FREQUENCY = 2E9


# ************************* Memory Parameters  *************************

# Memory throughput at different levels of the heirarchy (index 0 is L1 cache,
# etc.).
L1_THROUGHPUT   = 128E9
L2_THROUGHPUT   = 64E9
L3_THROUGHPUT   = 32E9
MEM_THROUGHPUT  = 4E9

# Cache sizes in *blocks/lines*
L1_SIZE         = 500
L2_SIZE         = 4000
L3_SIZE         = 62500

# Cache line size in bytes.
CACHE_LINE_SIZE = 64

# Memory Latencies
L1_LATENCY = 1
L2_LATENCY = 7
L3_LATENCY = 45
MEM_LATENCY = 100

# ************************* Instruction Parameters *************************

# Latency of a standard binary op (+ - / * >= etc.)
BINOP_LATENCY = 1

# Latency of an atomic add with no contention.
ATOMICADD_LATENCY = 3
# Penalty of contention for an atomic add.
ATOMICADD_PENALTY = 10

# Given a constant condition, The number of branches fall a certain way for the
# branch predictor to predict it correctly subsequently.
BRANCHPRED_PREDICTABLE_IT_DIST = 10

# The latency of executing a branching instruction.
BRANCHPRED_LATENCY = 3

# The branch misprediction penalty. The penalty is an expected value; when
# selectivity is closer to 0.5, the penalty is closer to the full
# BRANCHPRED_LATENCY.
def BRANCH_MISPREDICT_PENALTY(selectivity):
    # Returns a parabola whose maxima is at at s=0.5
    return (-1. * pow(selectivity * 2.0 - 1., 2.) + 1.) * BRANCHPRED_LATENCY


