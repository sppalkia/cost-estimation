/**
 * An implementation of kmeans in C.
 * Based on http://cs.smu.ca/~r_zhang/code/kmeans.c.
 *
 * See README for original license.
 */

#ifdef __linux__
#undef _BSD_SOURCE 
#define _BSD_SOURCE 500
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#include <stdlib.h>
#include <stdio.h>

#include <math.h>
#include <float.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <sys/time.h>

#include <omp.h>

#define BS 4000
#define THREADS 4

// Changes the width.
typedef float FLOAT;

const int n = 500 * 1000;
const int dim = 2;
const int k = 500 * 1000;
const int b = 1000;
const int iters = 1;

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

// Generated data parameters.
struct gen_data {
    FLOAT *data;
    int n;          // Number of points.
    int m;          // Dimension of points.
    int k;          // Number of clusters.
    int iters;      // Iterations to run.
};

/** Finds the distance between two vectors a and b with dim elements each.
 *
 * @param a the first vector
 * @param b the second vector
 * @param dim the dimension of the two vectors
 *
 * @return the euclidian distance between the two vectors.
 */
FLOAT distance_btwn(FLOAT *a, FLOAT *b, int dim) {
    FLOAT distance = 0;
    // TODO fix - eliminated exponent for faster experiments for now.
    for (int j = 0; j < dim; j++) {
        distance += b[j] - a[j];
    }
    return distance;
}

/** Runs the kmeans clustering algorithm on the given input data.
 * This function prints the runtime at the end of execution, not
 * counting initialization time/cleanup.
 *
 * @param d the input parameters.
 *
 * @return the labels assigned to the data points d->data. labels[i]
 * denotes the label of d->data[i].
 */
void k_means(struct gen_data *d) {

    FLOAT *data = d->data;
    /*
    int n = d->n;
    int dim = d->m;
    int k = d->k;
    int iterations = d->iters;
    */

    // Centroids.
    FLOAT *c = (FLOAT *)calloc(k * dim, sizeof(FLOAT));

    // Size of each cluster (thread-local state).
    int **all_counts = (int **)malloc(THREADS * sizeof(int **));
    for (int i = 0; i < THREADS; i++) {
        all_counts[i] = (int *)calloc(k, sizeof(int));
    }

    // Temporary centroids (thread-local state).
    FLOAT **all_c1 = (FLOAT **)malloc(THREADS * sizeof(FLOAT **));
    for (int i = 0; i < THREADS; i++) {
        all_c1[i] = (FLOAT *)calloc(k * dim, sizeof(FLOAT));
    }

    // For timing.
    struct timeval start, end, diff;

    // Sanity checks.
    assert(data && k > 0 && dim > 0 && iters >= 0);

    // Initialize.
    // Indexes into data to choose initialization points.
    int h = 0;
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < dim; j++) {
            c[i*dim + j] = data[h*dim + j];
        }
        h += n / k;
    }

    // Begin timing after iterations.
    gettimeofday(&start, 0);

    for (int iter = 0; iter < iters; iter++) {

        #pragma omp parallel for
        for (int t = 0; t < THREADS; t++) {  
            int nstart = t * (n / THREADS);
            int nend = nstart + (n / THREADS);
            if (t == THREADS - 1) {
                nend = n;
            }

            int *counts = all_counts[t];
            FLOAT *c1 = all_c1[t];

            // Clear old counts and temp centroids.
            memset(counts, 0, sizeof(int) * k);
            memset(c1, 0, sizeof(FLOAT) * dim * k);

            for (int i = nstart; i < nend; i++) {
                FLOAT *point = &data[i*dim];
                FLOAT min_distance = DBL_MAX;
                int label = 0;
                for (int j = 0; j < k; j++) {
                    FLOAT distance = distance_btwn(point, &c[j*dim], dim);
                    if (distance < min_distance) {
                        label = j;
                        min_distance = distance;
                    }
                }

                FLOAT *cent = &c1[label*dim];
                for (int j = 0; j < dim; j++) {
                    cent[j] += point[j];
                }
                counts[label]++;
            }
        }

        // Compute the final centroids (serial merge step).
        int *total_counts = all_counts[0];
        FLOAT *total_c1 = all_c1[0];
        for (int i = 0; i < k; i++) {
            for (int t = 1; t < THREADS; t++) {
                // Aggregate the counts and c1s.
                total_counts[i] += all_counts[t][i];
                for (int j = 0; j < dim; j++) {
                    total_c1[i*dim + j] += all_c1[t][i*dim + j];
                }
            }

            // Update the actual centroids.
            for (int j = 0; j < dim; j++) {
                if (total_counts[i] > 0) {
                    c[i*dim + j] = total_c1[i*dim +j] / total_counts[i];
                } else {
                    c[i*dim+j] = total_c1[i*dim+j];
                }
            }
        }
    }

    // Finish timing.
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Standard: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec,
            c[0]);

    // Make Parsing a bit easier but keep above line for backwards
    // compatibility with plotting script
    // Line Format:
    // Standard >> n,m,k,i,time,result
    printf(">>> S: %d,%d,%d,%d,%ld.%06ld,%d\n",
            d->n, d->m, d->k, d->iters,
            (long) diff.tv_sec, (long) diff.tv_usec,
            c[0]);

    // Cleanup.
    free(c);
    for (int i = 0; i < THREADS; i++) {
        free(all_c1[i]);
        free(all_counts[i]);
    }
    free(all_c1);
    free(all_counts);
}

int *k_means_blocked(struct gen_data *d) {

    FLOAT *data = d->data;
    /*
    int n = d->n;
    int dim = d->m;
    int k = d->k;
    int iterations = d->iters;
    */

    // Centroids.
    FLOAT *c = (FLOAT *)calloc(k * dim, sizeof(FLOAT));

    // Size of each cluster (thread-local state).
    int **all_counts = (int **)malloc(THREADS * sizeof(int **));
    for (int i = 0; i < THREADS; i++) {
        all_counts[i] = (int *)calloc(k, sizeof(int));
    }

    // Temporary centroids (thread-local state).
    FLOAT **all_c1 = (FLOAT **)malloc(THREADS * sizeof(FLOAT **));
    for (int i = 0; i < THREADS; i++) {
        all_c1[i] = (FLOAT *)calloc(k * dim, sizeof(FLOAT));
    }

    // For timing.
    struct timeval start, end, diff;

    // Sanity checks.
    assert(data && k > 0 && dim > 0 && iters >= 0);

    // Initialize.
    // Indexes into data to choose initialization points.
    int h = 0;
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < dim; j++) {
            c[i*dim + j] = data[h*dim + j];
        }
        h += n / k;
    }

    // Begin timing after iterations.
    gettimeofday(&start, 0);

    for (int iter = 0; iter < iters; iter++) {

        #pragma omp parallel for
        for (int t = 0; t < THREADS; t++) {  
            int nstart = t * (n / THREADS);
            int nend = nstart + (n / THREADS);
            if (t == THREADS - 1) {
                nend = n;
            }

            int *counts = all_counts[t];
            FLOAT *c1 = all_c1[t];

            // Clear old counts and temp centroids.
            memset(counts, 0, sizeof(int) * k);
            memset(c1, 0, sizeof(FLOAT) * dim * k);

            for (int i = nstart; i < nend; i += BS) {
                // Set the min_distances for this block.
                FLOAT min_distance[BS];
                int labels[BS];
                for (int q = 0; q < BS; q++) {
                    min_distance[q] = DBL_MAX;
                    labels[q] = 0;
                }

                int end = min(i + BS, n);

                // Find centroids for this block.
                for (int j = 0; j < k; j++) {
                    FLOAT *cent = &c[j*dim];
                    for (int ii = i; ii < end; ii++) {
                        // Find the closest cluster for the current point.
                        FLOAT distance = distance_btwn(&data[ii*dim], cent, dim);
                        if (distance < min_distance[ii-i]) {
                            labels[ii-i] = j;
                            min_distance[ii-i] = distance;
                        }
                    }
                }

                // Update the temporary centroids for this batch.
                for (int ii = i; ii < end; ii++) {
                    int label = labels[ii-i];
                    FLOAT *point = &data[ii*dim];
                    FLOAT *cent = &c1[label*dim];
                    for (int j = 0; j < dim; j++) {
                        cent[j] += point[j];
                    }
                    counts[label]++;
                }
            }

        }

        // Compute the final centroids (serial merge step).
        int *total_counts = all_counts[0];
        FLOAT *total_c1 = all_c1[0];
        for (int i = 0; i < k; i++) {
            for (int t = 1; t < THREADS; t++) {
                // Aggregate the counts and c1s.
                total_counts[i] += all_counts[t][i];
                for (int j = 0; j < dim; j++) {
                    total_c1[i*dim + j] += all_c1[t][i*dim + j];
                }
            }

            // Update the actual centroids.
            for (int j = 0; j < dim; j++) {
                if (total_counts[i] > 0) {
                    c[i*dim + j] = total_c1[i*dim +j] / total_counts[i];
                } else {
                    c[i*dim+j] = total_c1[i*dim+j];
                }
            }
        }
    }

    // Finish timing.
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Blocked: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec,
            c[0]);

    // Make Parsing a bit easier but keep above line for backwards
    // compatibility with plotting script
    // Line Format:
    // Blocked >> n,m,k,i,time,result
    printf(">>> B: %d,%d,%d,%d,%ld.%06ld,%d\n",
            d->n, d->m, d->k, d->iters,
            (long) diff.tv_sec, (long) diff.tv_usec,
            c[0]);

    // Cleanup.
    free(c);
    for (int i = 0; i < THREADS; i++) {
        free(all_c1[i]);
        free(all_counts[i]);
    }
    free(all_c1);
    free(all_counts);
}

/** Generates input data for the k-means test.
 *
 * @param n the number of points.
 * @param m the dimension of the points.
 * @param k the number of clusters.
 */
struct gen_data generate_data(int n, int m, int k, int iters) {
    struct gen_data d;
    d.n = n;
    d.m = m;
    d.k = k;
    d.iters = iters;

    printf("Generating data (%.1lf MB)...\n",
            n * m * sizeof(FLOAT) / 1024.0 / 1024.0
            );

    // Generate random data.
    d.data = (FLOAT *)malloc(sizeof(FLOAT) * n * m);
    for (int i = 0; i < n; i++) {
        FLOAT seed = random();
        for (int j = 0; j < m; j++) {
            d.data[i*m + j] = seed + j;
        }
    }

    printf("Done generating data\n");

    return d;
}

int main(int argc, char **argv) {
    // Default values.
    /*
    size_t n = 60 * 1000;
    int m = 2;
    int k = 60 * 1000;
    int iters = 1;

    int ch;
    while ((ch = getopt(argc, argv, "n:m:k:i:")) != -1) {
        switch (ch) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'm':
                m = atoi(optarg);
                break;
            case 'k':
                k = atoi(optarg);
                break;
            case 'i':
                iters = atoi(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }
    */

    // Sanity checks.
    assert(n > 0);
    assert(dim > 0);
    assert(k > 0);
    assert(iters > 0);

    printf("n=%zu, m=%d, k=%d, iters=%d\n", n, dim, k, iters);

    struct gen_data d = generate_data(n, dim, k, iters);

    // Standard k-means.
    k_means(&d);
    k_means_blocked(&d);

    return 0;
}
