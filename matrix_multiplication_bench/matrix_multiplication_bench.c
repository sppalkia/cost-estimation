/*
 * A test utility to check how our model captures simple blocking, which
 * provides better cache behavior.
 *
 * This test runs the following query:
 *
 *  for i in range(n):
 *    for j in range(n):
 *      for k in range(n):
 *        C[i][j] += (A[i][k] * B[k][j])
 *
 * - n is a configurable parameter.
 *
 * The nested loops above can be run un-changed, or blocked (to make better
 * use of cache locality). Here, we regard block_size as another parameter.
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

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

// Represents generated data.
struct gen_data {
    int *A;
    int *B;
    int *C;

    size_t block_size;
    size_t n;
};

void unblocked_matrix_multiplication(struct gen_data *d) {
    for (int i = 0; i < d->n; i++) {
        for (int j = 0; j < d->n; j++) {
            for (int k = 0; k < d->n; k++) {
                d->C[i*d->n + j] += (d->A[i*d->n + k] * d->B[k*d->n + j]);
            }
        }
    }
}

void transposed_matrix_multiplication(struct gen_data *d) {
    for (int i = 0; i < d->n; i++) {
        for (int j = 0; j < d->n; j++) {
            for (int k = 0; k < d->n; k++) {
                d->C[i*d->n + j] += (d->A[i*d->n + k] * d->B[j*d->n + k]);
            }
        }
    }
}

void blocked_matrix_multiplication(struct gen_data *d) {
    for (int kk = 0; kk < d->n; kk += d->block_size) {
        for (int jj = 0; jj < d->n; jj += d->block_size) {
            for (int i = 0; i < d->n; i++) {
                for (int j = jj; j < min(jj + d->block_size, d->n); j++) {
                    for (int k = kk; k < min(kk + d->block_size, d->n); k++) {
                        d->C[i*d->n + j] += (d->A[i*d->n + k] * d->B[k*d->n + j]);
                    }
                }
            }
        }
    }
}

long compute_sum(struct gen_data *d) {
    long sum = 0;
    for (int i = 0; i < d->n; i++) {
        for (int j = 0; j < d->n; j++) {
            sum += d->C[i*d->n + j];
        }
    }
    return sum;
}

struct gen_data load_data(size_t n, size_t block_size) {
    struct gen_data d;
    d.n = n;
    d.block_size = block_size;
    d.A = (int *)malloc(sizeof(int) * n * n);
    d.B = (int *)malloc(sizeof(int) * n * n);
    d.C = (int *)malloc(sizeof(int) * n * n);

    for (int i = 0; i < (n * n); i++) {
        d.A[i] = random() % 100;
        d.B[i] = random() % 100;
        d.C[i] = 0;
    }

    return d;
}

// Implementations of data generator given number of times the array should
// be passed over, and the number of elements in the array.
int main(int argc, char **argv) {

    // Default values
    // Width / height of matrices.
    // For now, we're dealing with square matrices.
    int n = (1E5 / sizeof(int));
    int block_size = 128;

    int ch;
    while ((ch = getopt(argc, argv, "n:b:")) != -1) {
        switch (ch) {
            case 'n':
                n = atof(optarg);
                break;
            case 'b':
                block_size = atof(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }

    printf("n=%d, block_size=%d\n", n, block_size); 

    struct gen_data d = load_data(n, block_size);
    long sum;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    transposed_matrix_multiplication(&d);
    gettimeofday(&end, 0);
    sum = compute_sum(&d);
    timersub(&end, &start, &diff);
    printf("Transposed: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    // Prevents caching effects.
    free(d.A);
    d = load_data(n, block_size);

    gettimeofday(&start, 0);
    unblocked_matrix_multiplication(&d);
    gettimeofday(&end, 0);
    sum = compute_sum(&d);
    timersub(&end, &start, &diff);
    printf("Unblocked: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    // Prevents caching effects.
    free(d.A);
    d = load_data(n, block_size);

    gettimeofday(&start, 0);
    blocked_matrix_multiplication(&d);
    gettimeofday(&end, 0);
    sum = compute_sum(&d);
    timersub(&end, &start, &diff);
    printf("Blocked: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    return 0;
}
