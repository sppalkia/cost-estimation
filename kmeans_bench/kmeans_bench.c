/*****
 ** kmeans.c
 ** - a simple k-means clustering routine
 ** - returns the cluster labels of the data points in an array
 ** - here's an example
 **   extern int *k_means(float**, int, int, int, float, float**);
 **   ...
 **   int *c = k_means(data_points, num_points, dim, 20, 1e-4, 0);
 **   for (i = 0; i < num_points; i++) {
 **      printf("data point %d is in cluster %d\n", i, c[i]);
 **   }
 **   ...
 **   free(c);
 ** Parameters
 ** - array of data points (float **data)
 ** - number of data points (int n)
 ** - dimension (int m)
 ** - desired number of clusters (int k)
 ** - error tolerance (float t)
 **   - used as the stopping criterion, i.e. when the sum of
 **     squared euclidean distance (standard error for k-means)
 **     of an iteration is within the tolerable range from that
 **     of the previous iteration, the clusters are considered
 **     "stable", and the function returns
 **   - a suggested value would be 0.0001
 ** - output address for the final centroids (float **centroids)
 **   - user must make sure the memory is properly allocated, or
 **     pass the null pointer if not interested in the centroids
 ** References
 ** - J. MacQueen, "Some methods for classification and analysis
 **   of multivariate observations", Fifth Berkeley Symposium on
 **   Math Statistics and Probability, 281-297, 1967.
 ** - I.S. Dhillon and D.S. Modha, "A data-clustering algorithm
 **   on distributed memory multiprocessors",
 **   Large-Scale Parallel Data Mining, 245-260, 1999.
 ** Notes
 ** - this function is provided as is with no warranty.
 ** - the author is not responsible for any damage caused
 **   either directly or indirectly by using this function.
 ** - anybody is free to do whatever he/she wants with this
 **   function as long as this header section is preserved.
 ** Created on 2005-04-12 by
 ** - Roger Zhang (rogerz@cs.dal.ca)
 ** Modifications
 ** -
 ** Last compiled under Linux with gcc-3
 */

#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include <float.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

struct gen_data {
    float *data;
    int n;
    int m;
    int k;
    int iters;
};

/** Finds the distance between two vectors a and b with dim elements each.
 *
 * @param a the first vector
 * @param b the second vector
 * @param dim the dimension of the two vectors
 *
 * @return the euclidian distance between the two vectors.
 */
float distance_btwn(float *a, float *b, int dim) {
    float distance = 0;
    for (int j = dim; j-- > 0; distance += pow(b[j] - a[j], 2));
    return distance;
}

/** TODO
 *
 */
int *k_means(struct gen_data *d) {

    float *data = d->data;
    int n = d->n;
    int dim = d->m;
    int k = d->k;
    int iterations = d->iters;

    // Output cluster labels for each point.
    int *labels = (int *)calloc(n, sizeof(int));
    // Size of each cluster.
    int *counts = (int *)calloc(k, sizeof(int));
    // Centroids.
    float *c = (float *)calloc(k * dim, sizeof(float));
    // Temporary centroids.
    float *c1 = (float *)calloc(k * dim, sizeof(float));

    // Sanity checks.
    assert(data && k > 0 && k <= n && dim > 0 && iterations >= 0);

    // Initialize.
    // Indexes into data to choose initialization points.
    int h = 0;
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < dim; j++) {
            c[i*dim + j] = data[h*dim + j];
        }
        h += n / k;
    }

    while (iterations != 0) {
        // Clear old counts and temp centroids.
        memset(counts, 0, sizeof(int) * k);
        memset(c1, 0, sizeof(float) * dim * k);

        for (int i = 0; i < n; i++) {
            float *point = &data[i*dim];

            // Find the closest cluster for the current point.
            float min_distance = DBL_MAX;
            for (int j = 0; j < k; j++) {
                float distance = distance_btwn(point, &c[j*dim], dim);
                if (distance < min_distance) {
                    labels[i] = j;
                    min_distance = distance;
                }            
            }

            for (int j = 0; j < dim; j++) {
                int label = labels[i];
                // TODO wat make sure this is right...
                c1[label*dim + j] += point[j];
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
struct gen_data generate_data(int n, int m, int k) {
    struct gen_data d;
    d.n = n;
    d.m = m;
    d.k = k;

    // Generate random data.
    d.data = (float *)malloc(sizeof(float) * n * m);
    for (int i = 0; i < n; i++) {
        float seed = random();
        for (int j = 0; j < m; j++) {
            d.data[i*m + j] = seed + j;
        }
    }
    return d;
}

int main(int argc, char **argv) {

    // Default values.
    size_t n = 1e6;
    int m = 100;
    int k = 5;
    int iters = 10;

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

    struct gen_data d = generate_data(n, m, k);
    int *labels = k_means(&d);

    return 0;
}
