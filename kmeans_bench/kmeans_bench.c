/**
 * An implementation of kmeans in C.
 * Based on http://cs.smu.ca/~r_zhang/code/kmeans.c.
 *
 * See README for original license.
 */

#include <stdlib.h>
#include <stdio.h>

#include <math.h>
#include <float.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <sys/time.h>

#define BS 16

// Changes the width.
typedef double FLOAT;

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
    for (int j = dim; j-- > 0; distance += b[j] - a[j]);
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
int *k_means(struct gen_data *d) {

    FLOAT *data = d->data;
    int n = d->n;
    int dim = d->m;
    int k = d->k;
    int iterations = d->iters;

    // Output cluster labels for each point.
    int *labels = (int *)calloc(n, sizeof(int));
    // Size of each cluster.
    int *counts = (int *)calloc(k, sizeof(int));
    // Centroids.
    FLOAT *c = (FLOAT *)calloc(k * dim, sizeof(FLOAT));
    // Temporary centroids.
    FLOAT *c1 = (FLOAT *)calloc(k * dim, sizeof(FLOAT));

    // For timing.
    struct timeval start, end, diff;

    // Sanity checks.
    assert(data && k > 0 && dim > 0 && iterations >= 0);

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

    while (iterations != 0) {
        // Clear old counts and temp centroids.
        memset(counts, 0, sizeof(int) * k);
        memset(c1, 0, sizeof(FLOAT) * dim * k);

        for (int i = 0; i < n; i++) {
            FLOAT *point = &data[i*dim];

            // Find the closest cluster for the current point.
            FLOAT min_distance = DBL_MAX;
            for (int j = 0; j < k; j++) {
                FLOAT distance = distance_btwn(point, &c[j*dim], dim);
                if (distance < min_distance) {
                    labels[i] = j;
                    min_distance = distance;
                }
            }

            int label = labels[i];
            FLOAT *cent = &c1[label*dim];
            for (int j = 0; j < dim; j++) {
                cent[j] += point[j];
            }
            counts[label]++;
        }

        // Update all centroids.
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < dim; j++) {
                if (counts[i] > 0) {
                    c[i*dim + j] = c1[i*dim +j] / counts[i];
                } else {
                    c[i*dim+j] = c1[i*dim+j];
                }
            }
        }

        // Update iterations.
        iterations--;
    }

    // Finish timing.
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Standard: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec,
            labels[0]);

    // Make Parsing a bit easier but keep above line for backwards
    // compatibility with plotting script
    // Line Format:
    // Blocked >> n,m,k,i,time,result
    printf(">>> S: %d,%d,%d,%d,%ld.%06ld,%d\n",
            d->n, d->m, d->k, d->iters,
            (long) diff.tv_sec, (long) diff.tv_usec,
            labels[0]);

    // Cleanup.
    free(c);
    free(c1);
    free(counts);

    return labels;
}

/** Runs the kmeans clustering algorithm on the given input data.
 * This function prints the runtime at the end of execution, not
 * counting initialization time/cleanup. This variant blocks points
 * and centroids in the cache.
 *
 * @param d the input parameters.
 *
 * @return the labels assigned to the data points d->data. labels[i]
 * denotes the label of d->data[i].
 */
int *k_means_blocked(struct gen_data *d) {

    FLOAT *data = d->data;
    int n = d->n;
    int dim = d->m;
    int k = d->k;
    int iterations = d->iters;

    // Output cluster labels for each point.
    int *labels = (int *)calloc(n, sizeof(int));
    // Size of each cluster.
    int *counts = (int *)calloc(k, sizeof(int));
    // Centroids.
    FLOAT *c = (FLOAT *)calloc(k * dim, sizeof(FLOAT));
    // Temporary centroids.
    FLOAT *c1 = (FLOAT *)calloc(k * dim, sizeof(FLOAT));

    // For timing.
    struct timeval start, end, diff;

    // Sanity checks.
    assert(data && k > 0 && dim > 0 && iterations >= 0);

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

    while (iterations != 0) {
        // Clear old counts and temp centroids.
        memset(counts, 0, sizeof(int) * k);
        memset(c1, 0, sizeof(FLOAT) * dim * k);

        for (int i = 0; i < n; i += BS) {
            // Set the min_distances for this block.
            FLOAT min_distance[BS];
            for (int q = 0; q < BS; q++) {
                min_distance[q] = DBL_MAX;
            }

            int end = min(i + BS, n);

            // Find centroids for this block.
            for (int j = 0; j < k; j++) {
                FLOAT *cent = &c[j*dim];
                for (int ii = i; ii < end; ii++) {
                    // Find the closest cluster for the current point.
                    FLOAT distance = distance_btwn(&data[ii*dim], cent, dim);
                    if (distance < min_distance[ii-i]) {
                        labels[ii] = j;
                        min_distance[ii-i] = distance;
                    }
                }
            }

            // Update the temporary centroids for this batch.
            for (int ii = i; ii < i + BS; ii++) {
                int label = labels[ii];
                FLOAT *point = &data[ii*dim];
                FLOAT *cent = &c1[label*dim];
                for (int j = 0; j < dim; j++) {
                    cent[j] += point[j];
                }
                counts[label]++;
            }
        }

        // Update all centroids.
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < dim; j++) {
                if (counts[i] > 0) {
                    c[i*dim + j] = c1[i*dim +j] / counts[i];
                } else {
                    c[i*dim+j] = c1[i*dim+j];
                }
            }
        }

        // Update iterations.
        iterations--;
    }

    // Finish timing.
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Blocked: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec,
            labels[0]);

    // Make Parsing a bit easier but keep above line for backwards
    // compatibility with plotting script
    // Line Format:
    // Blocked >> n,m,k,i,time,result
    printf(">>> B: %d,%d,%d,%d,%ld.%06ld,%d\n",
            d->n, d->m, d->k, d->iters,
            (long) diff.tv_sec, (long) diff.tv_usec,
            labels[0]);

    // Cleanup.
    free(c);
    free(c1);
    free(counts);

    return labels;
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

    // Generate random data.
    d.data = (FLOAT *)malloc(sizeof(FLOAT) * n * m);
    for (int i = 0; i < n; i++) {
        FLOAT seed = random();
        for (int j = 0; j < m; j++) {
            d.data[i*m + j] = seed + j;
        }
    }
    return d;
}

int main(int argc, char **argv) {
    // Default values.
    size_t n = 8192;
    int m = 1024;
    int k = 4096;
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

    // Sanity checks.
    assert(n > 0);
    assert(m > 0);
    assert(k > 0);
    assert(iters > 0);

    printf("n=%zu, m=%d, k=%d, iters=%d\n", n, m, k, iters);

    struct gen_data d = generate_data(n, m, k, iters);

    // Standard k-means.
    k_means(&d);
    k_means_blocked(&d);

    return 0;
}
