/*
 * A test utility to check how our model captures simple nested loops.
 *
 * This test runs the following query:
 *
 *  for i in range(k):
 *      for each element in A:
 *          sum += A[i]
 *
 * - n is a configurable parameter.
 * - k, the number of times the entire array is looped over, is also a
 *   configurable parameter.
 *
 * The loop can be run un-changed, or blocked (to make better use of cache
 * locality), or with loop order inverted.
 *
 * The cost model should provide an ordering equivalent to the performance
 * of the various loops.
 *
 */

#ifdef __linux__
#define _BSD_SOURCE 500
#define _POSIX_C_SOURCE 2
#endif

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>


// Represents generated data.
struct gen_data {
    int *A;

    size_t iters;
    size_t n;
};

int *no_materialization_query(struct gen_data *d) {
    int *res = (int *)malloc(sizeof(int) * d->n);
    for (int iter = 0; iter < d->iters; iter++) {
        int k = 0;
        for (int i = 0; i < d->n; i++) {
            if (d->A[i] > 50) {
                res[k++] = d->A[i];
            }
        }
    }
    return res;
}

int *materialization_query(struct gen_data *d) {
    bool *pred = (bool *)malloc(sizeof(bool) * d->n);
    int *res = (int *)malloc(sizeof(int) * d->n);
    for (int iter = 0; iter < d->iters; iter++) {
        for (int i = 0; i < d->n; i++) {
            pred[i] = (d->A[i] > 50);
        }
        int k = 0;
        for (int i = 0; i < d->n; i++) {
            if (pred[i]) {
                res[k++] = d->A[i];
            }
        }
    }
    return res;
}

struct gen_data load_data(size_t n) {
    struct gen_data d;
    d.n = n;
    d.A = (int *)malloc(sizeof(int) * n);

    for (int i = 0; i < n; i++) {
        d.A[i] = random() % 100;
    }
    d.iters = 5;

    return d;
}

// Implementations of data generator given number of times the array should
// be passed over, and the number of elements in the array.
int main(int argc, char **argv) {

    // Default values
    // Number of elements in array
    int n = (1E8 / sizeof(int));

    int ch;
    while ((ch = getopt(argc, argv, "n:")) != -1) {
        switch (ch) {
            case 'n':
                n = atof(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }

    printf("n=%d\n", n);

    struct gen_data d = load_data(n);
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    no_materialization_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("No materialization: %ld.%06ld (result=0.0)\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    // Prevents caching effects.
    free(d.A);
    d = load_data(n);

    gettimeofday(&start, 0);
    materialization_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Materialization: %ld.%06ld (result=0.0)\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    return 0;
}
