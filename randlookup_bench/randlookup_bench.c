
#ifdef __linux__
#define _BSD_SOURCE 500
#define _POSIX_C_SOURCE 2
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

struct gen_data {
    // Size of the randomly accessed array.
    size_t k;
    // Size of the vector.
    size_t n;
    // The randomly accessed array.
    int *R;
    // The vector.
    int *A;
};

void run_query(struct gen_data *d) {
    const int n = d->n;
    const int k = d->k;
    for (int i = 0; i < n; i++) {
        int index = d->A[i];
        d->R[index]++;
    }
}

/**
 * Creates a hash table of size k in which random lookups are performed,
 * along with a accessed vector of size n.
 */
struct gen_data load_data(size_t k, size_t n) {
    struct gen_data d;
    d.n = n;
    d.k = k;
    d.A = (int *)malloc(sizeof(int) * n);
    d.R = (int *)malloc(sizeof(int) * k);
    for (int i = 0; i < n; i++) {
        d.A[i] = random() % k;
    }
    return d;
}

// Implementations of data generator given number of times the array should
// be passed over, and the number of elements in the array.
int main(int argc, char **argv) {

    // Size of the randomly accessed vector.
    int k = 10;
    // Number of elements in array.
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

    assert(k > 0);
    assert(n > 0);
    printf("k=%d, n=%d\n", k, n); 

    struct gen_data d = load_data(k, n);
    run_query(&d);

    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    run_query(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Result: %ld.%06ld (result=%d)\n", (long) diff.tv_sec, (long) diff.tv_usec, d.R[0]);

    return 0;
}
