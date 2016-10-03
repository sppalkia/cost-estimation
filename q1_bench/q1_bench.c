/**
 * q1.c
 *
 * A test for varying parameters for queries similar to Q1.
 *
 */
#include <stdlib.h>
#include <string.h>

// Value for the predicate to pass.
#define PASS 100

// A bucket entry.
struct q1_entry {
    int sum_qty;
    int sum_base_price;
    int sum_disc_price;
    int sum_charge;
    int sum_discount;
    int count;
};

struct gen_data {
    // Number of lineitems in the table.
    int num_items;
    // Number of num_buckets/size of the hash table.
    int num_buckets;
    // Probability that the branch in the query will be taken.
    float prob;
    // The input data.
    struct lineitem *items;
    // The num_buckets.
    struct q1_entry *buckets;
};

struct lineitem {
    // The Q1 bucket this lineitem is clustered in.
    int bucket;

    // The branch condition (either PASS or FAIL).
    int shipdate;

    // Various fields used in the query.
    int quantity;
    int extendedprice;
    int discount;
    int tax;
};

void run_query(struct gen_data *d) {
    for (int i = 0; i < d->num_items; i++) {
        struct lineitem *item = &d->items[i];
        if (item->shipdate == PASS) {
            int bucket = item->bucket;
            struct q1_entry *e = &d->buckets[bucket];
            e->sum_qty += item->quantity;
            e->sum_base_price += item->extendedprice;
            e->sum_base_price += (item->extendedprice * (1 - item->discount));
            e->sum_charge +=
                (item->extendedprice * (1 - item->discount) * (1 + item->tax));
            e->sum_discount += item->discount;
            e->count++;
        }
    }
}

struct gen_data generate_data(int num_items, int num_buckets, float prob) {
    struct gen_data d;

    d.num_items = num_items;
    d.num_buckets = num_buckets;
    d.prob = prob;

    d.items = (struct lineitem *)malloc(sizeof(struct lineitem) * num_items);
    d.buckets = (struct q1_entry *)malloc(sizeof(struct q1_entry) * num_buckets);

    int pass_thres = (int)(prob * 100.0);
    for (int i = 0; i < d.num_items; i++) {
        struct lineitem *item = &d.items[i];
        if (random() % 100 <= pass_thres) {
            item->shipdate = PASS;
        } else {
            item->shipdate = 0;
        }
        // TODO: Make the parameterized as well? e.g. a distribution.
        item->bucket = random() % num_buckets;
    }

    // 0 out the num_buckets.
    memset(d.buckets, 0, sizeof(struct q1_entry) * num_buckets);
    return d;
}

int main(int argc, char **argv) {
    return 0;
}

