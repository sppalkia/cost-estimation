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
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <omp.h>

// Value for the predicate to pass.
#define PASS 100

#ifndef NUM_PARALLEL_THREADS
    #define NUM_PARALLEL_THREADS 4
#endif

// A bucket entry.
struct q3_entry {
    int32_t revenue;

    // Pad to 32 bytes.
    int8_t _pad[28];
};

// The generated input data.
struct gen_data {
    // Number of lineitems in the table.
    int32_t num_items;
    // Number of num_buckets/size of the hash table.
    int32_t num_buckets;
    // Probability that the branch in the query will be taken.
    float prob;
    // The input data.
    struct lineitem *items;
    // The num_buckets. This is a shared hash table across all threads.
    // Unused if using local bucket strategy.
    struct q3_entry *buckets;
};

// An input data item represented as in a row format.
struct lineitem {
    // The Q3 bucket this lineitem is clustered in.
    int32_t bucket;

    // The branch condition (either PASS or FAIL).
    int32_t shipdate;

    // Various fields used in the query.
    int32_t quantity;
    int32_t extendedprice;
    int32_t discount;
    int32_t tax;

    // So the structure is 1/2 a cache line exactly.
    long _pad;
};

/** Runs a worker for the query with a global shared hash table.
 *
 * TODO
 *
 * @param d the input data.
 * @param tid the thread ID of this worker.
 */
void run_query_global_table_helper(struct gen_data *d, int tid) {
    unsigned start = (d->num_items / NUM_PARALLEL_THREADS) * tid;
    unsigned end = start + (d->num_items / NUM_PARALLEL_THREADS);
    if (end > d->num_items || tid == NUM_PARALLEL_THREADS - 1) {
        end = d->num_items;
    }
    for (int i = start; i < end; i++) {
        struct lineitem *item = &d->items[i];
        if (item->shipdate == PASS) {
            int bucket = item->bucket;
            struct q3_entry *e = &d->buckets[bucket];
#pragma omp atomic
            e->revenue +=
                (item->extendedprice * (1 - item->discount));
        }
    }
}

/** Runs a worker for the query with thread-local hash tables.
 *
 * @param d the input data.
 * @param buckets the local buckets this worker writes into.
 * @param tid the thread ID of this worker.
 */
void run_query_local_table_helper(struct gen_data *d, struct q3_entry *buckets, int tid) {
    unsigned start = (d->num_items / NUM_PARALLEL_THREADS) * tid;
    unsigned end = start + (d->num_items / NUM_PARALLEL_THREADS);
    if (end > d->num_items || tid == NUM_PARALLEL_THREADS - 1) {
        end = d->num_items;
    }

    for (int i = start; i < end; i++) {
        struct lineitem *item = &d->items[i];
        if (item->shipdate == PASS) {
            int bucket = item->bucket;
            struct q3_entry *e = &buckets[bucket];
            e->revenue += (item->extendedprice * (1 - item->discount));
        }
    }
}

/** Runs a the query with a global shared hash table.
 *
 * @param d the input data.
 */
void run_query_global_table(struct gen_data *d) {
#pragma omp parallel for
    for (int i = 0; i < NUM_PARALLEL_THREADS; i++) {
        run_query_global_table_helper(d, i);
    }
}

/** Runs a the query with thread-local hash tables which are merged at the end.
 *
 * @param d the input data.
 */
void run_query_local_table(struct gen_data *d) {
    struct q3_entry *buckets = (struct q3_entry *)malloc(
            sizeof(struct q3_entry) * d->num_buckets * NUM_PARALLEL_THREADS);
    memset(buckets, 0, sizeof(struct q3_entry) * d->num_buckets * NUM_PARALLEL_THREADS);

#pragma omp parallel for
    for (int i = 0; i < NUM_PARALLEL_THREADS; i++) {
        run_query_local_table_helper(d, buckets + (d->num_buckets * i), i);
    }

    struct timeval start, end, diff;

    gettimeofday(&start, 0);

    // Aggregate the values.
    for (int i = 0; i < NUM_PARALLEL_THREADS; i++) {
        for (int j = 0; j < d->num_buckets; j++) {
            int b = i * d->num_buckets + j;
            d->buckets[j].revenue += buckets[b].revenue;
        }
    }

    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("\tagg-time: %ld.%06ld\n",
            (long) diff.tv_sec, (long) diff.tv_usec);
}

/** Generates input data.
 *
 * @param num_items the number of line items.
 * @param num_buckets the number of buckets we hash into.
 * @param prob the selectivity of the branch.
 * @return the generated data in a structure.
 */
struct gen_data generate_data(int num_items, int num_buckets, float prob) {
    struct gen_data d;

    d.num_items = num_items;
    d.num_buckets = num_buckets;
    d.prob = prob;

    d.items = (struct lineitem *)malloc(sizeof(struct lineitem) * num_items);
    d.buckets = (struct q3_entry *)malloc(sizeof(struct q3_entry) * num_buckets);

    int pass_thres = (int)(prob * 100.0);
    for (int i = 0; i < d.num_items; i++) {
        struct lineitem *item = &d.items[i];
        if (random() % 100 <= pass_thres) {
            item->shipdate = PASS;
        } else {
            item->shipdate = 0;
        }

        int seed = random();

        // Random values.
        item->quantity = seed;
        item->extendedprice = seed + 1;
        item->discount = seed + 2;
        item->tax = seed + 3;

        item->bucket = random() % num_buckets;
    }

    // 0 out the num_buckets.
    memset(d.buckets, 0, sizeof(struct q3_entry) * d.num_buckets);
    return d;
}

int main(int argc, char **argv) {
    // Number of bucket entries (default = 6, same as TPC-H Q1)
    // TODO: Set to default number of buckets for TPC-H Q3
    int num_buckets = 6;
    // Number of elements in array (should be >> cache size);
    int num_items = (1E8 / sizeof(int));
    // Approx. PASS probability.
    float prob = 0.01;

    int ch;
    while ((ch = getopt(argc, argv, "b:n:p:")) != -1) {
        switch (ch) {
            case 'b':
                num_buckets = atoi(optarg);
                break;
            case 'n':
                num_items = atoi(optarg);
                break;
            case 'p':
                prob = atof(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "invalid options");
                exit(1);
        }
    }

    // Check parameters.
    assert(num_buckets > 0);
    assert(num_items > 0);
    assert(prob >= 0.0 && prob <= 1.0);
    assert(num_buckets % 2 == 0);

    printf("n=%d, b=%d, p=%f\n", num_items, num_buckets, prob);

    struct gen_data d = generate_data(num_items, num_buckets, prob);
    long sum;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    run_query_local_table(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Local: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec,
            d.buckets[0].revenue);

    // Reset the buckets.
    memset(d.buckets, 0, sizeof(struct q3_entry) * d.num_buckets);

    gettimeofday(&start, 0);
    run_query_global_table(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Global: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec,
            d.buckets[0].revenue);

    return 0;
}
