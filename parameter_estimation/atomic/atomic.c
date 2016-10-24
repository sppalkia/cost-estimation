/**
 * q1.c
 *
 * A test for varying parameters for queries similar to Q1.
 *
 */

#ifdef __linux__
#define _BSD_SOURCE 500
#define _POSIX_C_SOURCE 2
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <omp.h>



// The generated input data.
struct gen_data {
    // Number of elements in the array.
    int32_t num_elements;

    // Arrays to operate on.
    int32_t *a;
    int32_t *b;
};

void run_atomic_version(struct gen_data *d) {
    for (int i = 0; i < d->num_elements; i++) {
#pragma omp atomic
        d->b[i] += d->a[i];
    }
}

void run_regular_version(struct gen_data *d) {
    for (int i = 0; i < d->num_elements; i++) {
        d->b[i] += d->a[i];
    }
}

void run_atomic_version_multiple_threads(struct gen_data *d, int num_parallel_threads) {
#pragma omp parallel for
    for (int i = 0; i < num_parallel_threads; i++) {
        run_atomic_version(d);
    }
}

void run_regular_version_multiple_threads(struct gen_data *d, int num_parallel_threads) {
#pragma omp parallel for
    for (int i = 0; i < num_parallel_threads; i++) {
        run_regular_version(d);
    }
}

/** Generates input data.
 *
 * @param num_elements the number of elements in the array.
 */
struct gen_data generate_data(int num_elements) {
    struct gen_data d;
    d.num_elements = num_elements;
    d.a = (int32_t *)malloc(sizeof(int32_t) * num_elements);
    d.b = (int32_t *)malloc(sizeof(int32_t) * num_elements);

    for (int i = 0; i < d.num_elements; i++) {
      d.a[i] = random();
      d.b[i] = random();
    }

    return d;
}

int main(int argc, char **argv) {
    // Number of elements in array (should be >> cache size);
    int num_elements = (1E8 / sizeof(int));

    int ch;
    while ((ch = getopt(argc, argv, "n:")) != -1) {
        switch (ch) {
            case 'n':
                num_elements = atoi(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }

    printf("n=%d\n", num_elements);

    struct gen_data d = generate_data(num_elements);
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    run_regular_version(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Regular with 1 thread: %ld.%06ld\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    gettimeofday(&start, 0);
    run_regular_version_multiple_threads(&d, 4);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Regular with 4 threads: %ld.%06ld\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    gettimeofday(&start, 0);
    run_regular_version_multiple_threads(&d, 8);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Regular with 8 threads: %ld.%06ld\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    gettimeofday(&start, 0);
    run_regular_version_multiple_threads(&d, 16);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Regular with 16 threads: %ld.%06ld\n\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    gettimeofday(&start, 0);
    run_atomic_version(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Atomic with 1 thread: %ld.%06ld\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    gettimeofday(&start, 0);
    run_atomic_version_multiple_threads(&d, 4);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Atomic with 4 threads: %ld.%06ld\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    gettimeofday(&start, 0);
    run_atomic_version_multiple_threads(&d, 8);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Atomic with 8 threads: %ld.%06ld\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    gettimeofday(&start, 0);
    run_atomic_version_multiple_threads(&d, 16);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Atomic with 16 threads: %ld.%06ld\n",
            (long) diff.tv_sec, (long) diff.tv_usec);

    return 0;
}
