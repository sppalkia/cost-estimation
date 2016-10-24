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
    // Number of orders in the table.
    int32_t num_orders;
    // Number of customers in the table.
    int32_t num_customers;

    // Number of num_buckets/size of the hash table.
    int32_t num_buckets;
    // Probability that the branch in the query will be taken.
    float prob;
    // Pointers to the input data.
    struct lineitems *items;
    struct orders *orders;
    struct customers *customers;
    // The num_buckets. This is a shared hash table across all threads.
    // Unused if using local bucket strategy.
    struct q3_entry *buckets;
};

// An input data item represented as in a column format.
struct lineitems {
    // The Q3 bucket this lineitem is clustered in.
    int32_t* bucket;

    // The branch condition (either PASS or FAIL).
    int32_t* shipdate;

    // Various fields used in the query.
    int32_t* orderkey;
    int32_t* quantity;
    int32_t* extendedprice;
    int32_t* discount;
    int32_t* tax;
};

struct orders {
    int32_t* orderkey;
    int32_t* custkey;
    int32_t* orderdate;
    int32_t* orderpriority;
    int32_t* shippriority;
};

struct customers {
    int32_t* mktsegment;
};

/** Runs a worker for the query with a global shared hash table.
 *
 * TODO
 *
 * @param d the input data.
 * @param tid the thread ID of this worker.
 */
int32_t run_query_global_table_helper(struct gen_data *d, int tid) {
    unsigned start = (d->num_items / NUM_PARALLEL_THREADS) * tid;
    unsigned end = start + (d->num_items / NUM_PARALLEL_THREADS);
    if (end > d->num_items || tid == NUM_PARALLEL_THREADS - 1) {
        end = d->num_items;
    }
    int32_t result = 0;

    struct lineitems *items = d->items;
    struct orders *orders = d->orders;
    struct customers *customers = d->customers;
    for (int i = start; i < end; i++) {
        int32_t order_idx = items->orderkey[i];
        int32_t customer_idx = orders->custkey[order_idx];
        int32_t orderdate = orders->orderdate[order_idx];
        int32_t mktsegment = customers->mktsegment[customer_idx];
        int32_t shipdate = items->shipdate[i];

        result += (orderdate + mktsegment);
        if (items->shipdate[i] == PASS) {
            int bucket = items->bucket[i];
            struct q3_entry *e = &d->buckets[bucket];
#pragma omp atomic
            e->revenue +=
                (items->extendedprice[i] * (1 - items->discount[i]));
        }
    }
    return result;
}

/** Runs a worker for the query with thread-local hash tables.
 *
 * @param d the input data.
 * @param buckets the local buckets this worker writes into.
 * @param tid the thread ID of this worker.
 */
int32_t run_query_local_table_helper(struct gen_data *d, struct q3_entry *buckets, int tid) {
    unsigned start = (d->num_items / NUM_PARALLEL_THREADS) * tid;
    unsigned end = start + (d->num_items / NUM_PARALLEL_THREADS);
    if (end > d->num_items || tid == NUM_PARALLEL_THREADS - 1) {
        end = d->num_items;
    }
    int32_t result = 0;

    struct lineitems *items = d->items;
    struct orders *orders = d->orders;
    struct customers *customers = d->customers;
    for (int i = start; i < end; i++) {
        int32_t order_idx = items->orderkey[i];
        int32_t customer_idx = orders->custkey[order_idx];
        int32_t orderdate = orders->orderdate[order_idx];
        int32_t mktsegment = customers->mktsegment[customer_idx];
        int32_t shipdate = items->shipdate[i];
        result += (orderdate + mktsegment);

        if (items->shipdate[i] == PASS) {
            int bucket = items->bucket[i];
            struct q3_entry *e = &buckets[bucket];
            e->revenue += (items->extendedprice[i] * (1 - items->discount[i]));
        }
    }
    return result;
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
struct gen_data generate_data(int num_items, int num_orders, int num_customers, int num_buckets, float prob) {
    struct gen_data d;

    d.num_items = num_items;
    d.num_orders = num_orders;
    d.num_customers = num_customers;

    d.num_buckets = num_buckets;
    d.prob = prob;

    d.items = (struct lineitems *)malloc(sizeof(struct lineitems));
    d.orders = (struct orders *)malloc(sizeof(struct orders));
    d.customers = (struct customers *)malloc(sizeof(struct customers));
    d.buckets = (struct q3_entry *)malloc(sizeof(struct q3_entry) * num_buckets);

    int pass_thres = (int)(prob * 100.0);
    struct lineitems *items = d.items;
    items->shipdate = (int32_t *)malloc(sizeof(int32_t) * num_items);
    items->orderkey = (int32_t *)malloc(sizeof(int32_t) * num_items);
    items->quantity = (int32_t *)malloc(sizeof(int32_t) * num_items);
    items->extendedprice = (int32_t *)malloc(sizeof(int32_t) * num_items);
    items->discount = (int32_t *)malloc(sizeof(int32_t) * num_items);
    items->tax = (int32_t *)malloc(sizeof(int32_t) * num_items);
    items->bucket = (int32_t *)malloc(sizeof(int32_t) * num_items);
    for (int i = 0; i < d.num_items; i++) {
        if (random() % 100 <= pass_thres) {
            items->shipdate[i] = PASS;
        } else {
            items->shipdate[i] = 0;
        }

        int seed = random();

        // Random values.
        items->quantity[i] = seed;
        items->extendedprice[i] = seed + 1;
        items->discount[i] = seed + 2;
        items->tax[i] = seed + 3;

        items->bucket[i] = random() % num_buckets;
    }

    struct orders *orders = d.orders;
    orders->orderkey = (int32_t *)malloc(sizeof(int32_t) * num_orders);
    orders->custkey = (int32_t *)malloc(sizeof(int32_t) * num_orders);
    orders->orderdate = (int32_t *)malloc(sizeof(int32_t) * num_orders);
    orders->orderpriority = (int32_t *)malloc(sizeof(int32_t) * num_orders);
    orders->shippriority = (int32_t *)malloc(sizeof(int32_t) * num_orders);

    for (int i = 0; i < d.num_orders; i++) {
        int seed = random();
        orders->orderkey[i] = random() % d.num_orders;
        orders->custkey[i] = random() % d.num_customers;
        orders->orderdate[i] = seed + 2;
        orders->orderpriority[i] = seed + 3;
        orders->shippriority[i] = seed + 4;
    }

    struct customers *customers = d.customers;
    customers->mktsegment = (int32_t *)malloc(sizeof(int32_t) * num_customers);
    for (int i = 0; i < d.num_customers; i++) {
        int seed = random();
        customers->mktsegment[i] = seed;
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
    int num_orders = (1E5 / sizeof(int));
    int num_customers = (1E4 / sizeof(int));
    // Approx. PASS probability.
    float prob = 0.01;

    int ch;
    while ((ch = getopt(argc, argv, "b:n:o:c:p:")) != -1) {
        switch (ch) {
            case 'b':
                num_buckets = atoi(optarg);
                break;
            case 'n':
                num_items = atoi(optarg);
                break;
            case 'o':
                num_orders = atoi(optarg);
                break;
            case 'c':
                num_customers = atoi(optarg);
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

    struct gen_data d = generate_data(num_items, num_orders, num_customers, num_buckets, prob);
    long sum;
    struct timeval start, end, diff;

    gettimeofday(&start, 0);
    run_query_local_table(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Local: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec,
            d.buckets[0].revenue);

    // For easy parsing
    printf(">>> L(result=%d):%ld.%06ld\t%d\t%d\t%f\n",
            d.buckets[0].revenue,
            (long) diff.tv_sec, (long) diff.tv_usec,
            num_items, num_buckets, prob);

    // Reset the buckets.
    memset(d.buckets, 0, sizeof(struct q3_entry) * d.num_buckets);

    gettimeofday(&start, 0);
    run_query_global_table(&d);
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    printf("Global: %ld.%06ld (result=%d)\n",
            (long) diff.tv_sec, (long) diff.tv_usec,
            d.buckets[0].revenue);

    // For easy parsing
    printf(">>> G(result=%d):%ld.%06ld\t%d\t%d\t%f\n",
            d.buckets[0].revenue,
            (long) diff.tv_sec, (long) diff.tv_usec,
            num_items, num_buckets, prob);

    return 0;
}
