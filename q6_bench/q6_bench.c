/*
 * A test utility to check how our model captures simple loops.
 *
 * This test runs the following query:
 *
 *  for each element in A:
 *      if (A[i] > 100):
 *          sum = V0[i] + V1[i] + ... + Vn[i]
 *
 * - n is a configurable parameter.
 * - s, the selectivity of the branch is also a configurable parameter.
 *
 * The loop can be run in a vectorized mode (where vector CPU instructions are
 * used) or in a scalar mode.
 *
 * The cost model should provide an ordering equivalent the the performance
 * of the various loops.
 *
 */

#ifdef __linux__
#define _BSD_SOURCE 500
#define _POSIX_C_SOURCE 2
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <immintrin.h>

#define PASS 200
#define FAIL (PASS+1)

// Represents generated data.
struct gen_data {
    int *A;
    int **V;

    size_t vs;
    size_t n;
};

long branched_scalar_query(struct gen_data *d) {
    long sum = 0;
    for (int i = 0; i < d->n; i++) {
        if (d->A[i] == PASS) {
            for (int j = 0; j < d->vs; j++) {
                sum += d->V[j][i];
            }
        }
    }
    return sum;
}

long nobranch_scalar_query(struct gen_data *d) {
    long sum = 0;
    for (int i = 0; i < d->n; i++) {
        long result = 0;
        for (int j = 0; j < d->vs; j++) {
            result += d->V[j][i];
        }
        sum += (d->A[i] == PASS) * result;
    }
    return sum;
}


long vector_pred_query(struct gen_data *d) {
  // Loop variables.
  size_t i, j;
  // Final sum/result.
  long sum = 0;

  // The vector used to check the PASS  condition.
  const __m256i v_pass = _mm256_set1_epi32(PASS);
 
  // The total sum.
  __m256i v_sum = _mm256_set1_epi32(0);
  // Used for aggergation.
  __m128i v_high, v_low;

  // For collecting sum for a single iteration.
  __m256i v_cur, v_j;
  // Condition and mask.
  __m256i v_cond, v_mask;

  for (i = 0; i+8 <= d->n; i += 8) {
    // Collect the sum for this set.
    v_cur = _mm256_set1_epi32(0);
    for (j = 0; j < d->vs; j++) {
      v_j = _mm256_lddqu_si256((const __m256i *)(d->V[j] + i));
      v_cur = _mm256_add_epi32(v_cur, v_j);
    }

    // Load the condition.
    v_cond = _mm256_lddqu_si256((const __m256i *)(d->A + i));
    // Check which lanes pass the condition, and zero the rest.
    v_cond = _mm256_cmpeq_epi32(v_cond, v_pass);

    // Take the AND of the summed elements and the mask to filter values,
    // and then add it to the sum.
    v_sum = _mm256_add_epi32(_mm256_and_si256(v_cond, v_cur), v_sum);
  }

  // Handle the fringe
  for (; i < d->n; i++) {
          long result = 0;
          for (int j = 0; j < d->vs; j++) {
                  result += d->V[j][i];
          }
          sum += (d->A[i] == PASS) * result;
  }

  // We can use three instructions here to collapse the eight values using a sum,
  // into two 32-bit values. Then we extract those two values from the vector and
  // add them into the final result.
  v_sum = _mm256_hadd_epi32(v_sum, _mm256_set1_epi32(0));
  v_high = _mm256_extracti128_si256(v_sum, 1);
  v_low = _mm256_castsi256_si128(v_sum);
  v_sum = _mm256_castsi128_si256(_mm_add_epi32(v_low, v_high));

  sum += _mm256_extract_epi32(v_sum, 0) + _mm256_extract_epi32(v_sum, 1);
  return sum;
}

struct gen_data load_data(size_t vs,
        size_t n,
        float sel) {

    if (sel > 1.0) {
        sel = 1.0;
    } else if (sel < 0.0) {
        sel = 0.0;
    }

    struct gen_data d;
    d.n = n;
    d.vs = vs;
    d.A = (int *)malloc(sizeof(int) * n);

    int check = (int)(100.0 * sel);
    for (int i = 0; i < n; i++) {
        if (random() % 100 <= check)  {
            d.A[i] = PASS;
        } else {
            d.A[i] = FAIL;
        }
    }

    const int value = 1;

    d.V = (int **)malloc(sizeof(int *) * vs);
    for (int i = 0; i < vs; i++) {
        d.V[i] = (int *)malloc(sizeof(int) * n);
        for (int j = 0; j < n; j++) {
            d.V[i][j] = value;
        }
    }

    return d;
}

// Implementations of data generator given selectivity.
int main(int argc, char **argv) {

    // Default values
    // Number of loads when predicate matches.
    int vs = 1;
    // Number of elements in array (should be >> cache size);
    int n = (1E8 / sizeof(int));
    // Approx. selectivity.
    float sel = 0.01;

    int ch;
    while ((ch = getopt(argc, argv, "n:v:s:")) != -1) {
        switch (ch) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'v':
                vs = atoi(optarg);
                break;
            case 's':
                sel = atof(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }

    printf("vs=%d, sel=%f\n", vs, sel); 

    struct gen_data d = load_data(vs, n, sel);
    long sum;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    sum = branched_scalar_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Branched: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    printf(">>> B(result=%ld):%ld.%06ld\t%d\t%d\t%f\n", sum,
            (long) diff.tv_sec, (long) diff.tv_usec, n, vs, sel);

    gettimeofday(&start, 0);
    sum = nobranch_scalar_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("No Branch: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    printf(">>> NB(result=%ld):%ld.%06ld\t%d\t%d\t%f\n", sum,
            (long) diff.tv_sec, (long) diff.tv_usec, n, vs, sel);

    gettimeofday(&start, 0);
    sum = vector_pred_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Vector: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    return 0;
}
