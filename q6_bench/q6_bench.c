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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

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
        memset_pattern4(d.V[i], &value, sizeof(int) * n);
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
    while ((ch = getopt(argc, argv, "v:s:")) != -1) {
        switch (ch) {
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

    gettimeofday(&start, 0);
    sum = nobranch_scalar_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("No Branch: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    return 0;
}
