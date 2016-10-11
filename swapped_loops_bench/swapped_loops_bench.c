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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>


#define BLOCK_SIZE 1024

// Represents generated data.
struct gen_data {
    int *A;

    size_t k;
    size_t n;
};

long unblocked_nested_loops_query(struct gen_data *d) {
    long sum = 0;
    for (int i = 0; i < d->k; i++) {
        for (int j = 0; j < d->n; j++) {
            if (d->A[j] > 50) {
                sum += d->A[j];
            }
        }
    }
    return sum;
}

long swapped_nested_loops_query(struct gen_data *d) {
    long sum = 0;
    for (int j = 0; j < d->n; j++) {
        for (int i = 0; i < d->k; i++) {
            if (d->A[j] > 50) {
                sum += d->A[j];
            }
        }
    }
    return sum;
}

struct gen_data load_data(size_t k,
        size_t n) {
    struct gen_data d;
    d.k = k;
    d.n = n;
    d.A = (int *)malloc(sizeof(int) * n);

    for (int i = 0; i < n; i++) {
        d.A[i] = random() % 100;
    }

    return d;
}

// Implementations of data generator given number of times the array should
// be passed over, and the number of elements in the array.
int main(int argc, char **argv) {

    // Default values
    // Number of times the entire array is passed over
    int k = 1;
    // Number of elements in array
    int n = (1E8 / sizeof(int));

    int ch;
    while ((ch = getopt(argc, argv, "k:n:")) != -1) {
        switch (ch) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'n':
                n = atof(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }

    printf("k=%d, n=%d\n", k, n); 

    struct gen_data d = load_data(k, n);
    long sum;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    sum = unblocked_nested_loops_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Original: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    // Prevents caching effects.
    free(d.A);
    d = load_data(k, n);

    gettimeofday(&start, 0);
    sum = swapped_nested_loops_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Interchanged: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    return 0;
}
