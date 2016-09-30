
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
    int items;
    // Number of buckets/size of the hash table.
    int buckets;
    // Probability that the branch in the query will be taken.
    float prob;
    // The input data.
    struct lineitem *items;
    // The buckets.
    struct q1_entry *entries;
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
    for (int i = 0; i < d->items; i++) {
        struct lineitem *item = d->items[i];
        if (item->shipdate[i] == PASS) {
            int bucket = item->bucket[i];
            struct q1_entry *e = d->buckets[bucket];
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

struct gen_data generate_data(int items, int buckets, float prob) {
    struct gen_data d;

    d.items = items;
    d.buckets = buckets;
    d.prob = prob;

    d.items = (struct lineitem *)malloc(sizeof(struct lineitem) * n);
    d.buckets = (struct q1_entry *)malloc(sizeof(struct q1_entry) * buckets);

    memset(d.buckets, 0, sizeof(struct q1_entry) * buckets);
}

