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
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>


#define BLOCK_SIZE 1024

// Represents generated data.
struct gen_data {
    float *A;
    float *B;

    size_t k;
    size_t n;
    float p;
};

// 1. compute dot product of d->k[i]
// 2. For each d->n[j]:
// 3.   If passes threshold:
// 4.     score += dot;
//
// map(A,
//  z := compute(a);
//  agg(filter(v, someFilter), current + z)
// )
//
//

#define DIM 100

float compute_i(float *Iarr) {
    double sum = 0.0;
    for (int i = 0; i < DIM; i++) {
        sum += pow(Iarr[i], 2);
    }
    return sqrt(sum);
}

float compute_j(float *Jarr) {
    return Jarr[0];
}

int compute_i_j(float *Iarr, float *Jarr, double i_norm) {
    double j_norm = compute_i(Jarr);
    double sum = 0.0;
    for (int i = 0; i < DIM; i++) {
        sum += Iarr[i] * Jarr[i]; 
    }

    return sum / (i_norm * j_norm);
}

long unswapped_nested_loops_query(struct gen_data *d) {
    long sum = 0;
    for (int i = 0; i < d->k; i++) {
        double z = compute_i(&d->B[i*DIM]); // One-time computation.
        for (int j = 0; j < d->n; j++) {
            if (compute_j(&d->A[j]) < d->p) { // Unpredictable branch.
                sum += compute_i_j(&d->B[i*DIM], &d->A[j*DIM], z);
            }
        }
    }
    return sum;
}

long swapped_nested_loops_query(struct gen_data *d) {
    long sum = 0;
    for (int j = 0; j < d->n; j++) {
        if (compute_j(&d->A[j*DIM]) < d->p) { 
            for (int i = 0; i < d->k; i++) {
                int z = compute_i(&d->B[i*DIM]); // Repeated computation.
                sum += compute_i_j(&d->B[i*DIM], &d->A[j*DIM], z);
            }
        }
    }
    return sum;
}

long cached_nested_loops_query(struct gen_data *d) {
    long sum = 0;
    // precompute norms.
    struct timeval start, end, diff;

    gettimeofday(&start, 0);

    float *norms  = (float *)malloc(sizeof(float) * d->k);
    for (int i = 0; i < d->k; i++) {
        norms[i] = compute_i(&d->B[i*DIM]); // Cache the computation.
    }

    gettimeofday(&end, 0);

    timersub(&end, &start, &diff);
    printf("\tNorms: %ld.%06ld (result=%ld)\n",
            (long) diff.tv_sec, (long) diff.tv_usec, sum);

    for (int j = 0; j < d->n; j++) {
        if (compute_j(&d->A[j*DIM]) < d->p) { 
        for (int i = 0; i < d->k; i++) {
                sum += compute_i_j(&d->B[i*DIM], &d->A[j*DIM], norms[i]);
            }
        }
    }
    free(norms);
    return sum;
}

struct gen_data load_data(size_t k,
        size_t n,
        float p) {
    struct gen_data d;
    d.k = k;
    d.n = n;

    p *= 100.0;
    d.p = p;

    d.A = (float *)malloc(sizeof(float) * n * DIM);
    d.B = (float *)malloc(sizeof(float) * k * DIM);

    for (int i = 0; i < n; i++) {
        d.A[i*DIM] = (float)(random() % 100);
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
    int n = 100;

    float p = 1.0;

    int ch;
    while ((ch = getopt(argc, argv, "k:n:p:")) != -1) {
        switch (ch) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'n':
                n = atof(optarg);
                break;
            case 'p':
                p = atof(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }

    printf("k=%d, n=%d, p=%f, dim=%d\n", k, n, p, DIM); 

    struct gen_data d = load_data(k, n, p);
    long sum;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    sum = unswapped_nested_loops_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Original (result=%ld):%ld.%06ld\tk=%d\tn=%d\tp=%f\tdim=%d\n",
            sum, (long) diff.tv_sec, (long) diff.tv_usec, k, n, p, DIM);

    // Prevents caching effects.
    free(d.A);
    free(d.B);
    d = load_data(k, n, p);

    gettimeofday(&start, 0);
    sum = swapped_nested_loops_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Interchanged (result=%ld):%ld.%06ld\tk=%d\tn=%d\tp=%f\tdim=%d\n",
            sum, (long) diff.tv_sec, (long) diff.tv_usec, k, n, p, DIM);

    // Prevents caching effects.
    free(d.A);
    free(d.B);
    d = load_data(k, n, p);

    gettimeofday(&start, 0);
    sum = cached_nested_loops_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Cached (result=%ld):%ld.%06ld\tk=%d\tn=%d\tp=%f\tdim=%d\n",
            sum, (long) diff.tv_sec, (long) diff.tv_usec, k, n, p, DIM);

    return 0;
}
