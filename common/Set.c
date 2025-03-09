#include "Set.h"
#include <stdlib.h>
#include <math.h>

struct Buckets {
    List **buckets;
    uint64_t num_of_buckets;
    size_t value_size;
    Hash hash;
    ValueComparator compare;
    FreeValueFunction free_value;
};

static void freeBuckets(struct Buckets *buckets, uint8_t deep) {
    if (deep)
        for (uint64_t i = 0; i < buckets->num_of_buckets; ++i) {
            deepClearList(buckets->buckets[i], buckets->free_value);
            free(buckets->buckets[i]);
        }
    else {
        for (uint64_t i = 0; i < buckets->num_of_buckets; ++i) {
            clearList(buckets->buckets[i]);
            free(buckets->buckets[i]);
        }
    }
    free(buckets->buckets);
    free(buckets);
}

static struct Buckets *newBuckets(
    uint64_t num_of_buckets,
    size_t value_size,
    Hash hash,
    ValueComparator compare,
    FreeValueFunction free_value
) {
    struct Buckets *buckets = malloc(sizeof(struct Buckets));
    if (!buckets) return NULL;
    buckets->buckets = malloc(num_of_buckets * sizeof(List *));
    if (!buckets->buckets) {
        free(buckets);
        return NULL;
    }
    buckets->num_of_buckets = num_of_buckets;
    buckets->value_size = value_size;
    buckets->hash = hash;
    buckets->compare = compare;
    buckets->free_value = free_value;
    for (uint64_t i = 0; i < num_of_buckets; ++i) {
        List *bucket = malloc(sizeof(List));
        if (!bucket) {
            buckets->num_of_buckets = i;
            freeBuckets(buckets, 0);
            return NULL;
        }
        buckets->buckets[i] = bucket;
    }
    return buckets;
}

static int pushBuckets(struct Buckets *buckets, void *value) {
    uint64_t index = buckets->hash(value) & buckets->num_of_buckets;
    if (deepContainsList(buckets->buckets[index], value, buckets->compare))
        return 0;
    if (pushList(buckets->buckets[index], value, buckets->value_size))
        return -1;
    return 0;
}

static void popBuckets(struct Buckets *buckets, void *value, uint8_t deep) {
    uint64_t index = buckets->hash(value) & buckets->num_of_buckets;
    uint64_t list_index = deepIndexOfList(buckets->buckets[index], value, buckets->compare);
    if (list_index >= getListSize(buckets->buckets[index])) return;
    value = popListAtIndex(buckets->buckets[index], list_index, 0);
    if (deep) buckets->free_value(value);
    free(value);
}

static uint8_t containsBuckets(struct Buckets *buckets, void *value) {
    uint64_t index = buckets->hash(value) & buckets->num_of_buckets;
    return deepContainsList(buckets->buckets[index], value, buckets->compare);
}

static uint64_t getNumOfBuckets(struct Buckets *buckets) {
    return buckets->num_of_buckets;
}

static size_t getBucketsValueSize(struct Buckets *buckets) {
    return buckets->value_size;
}

static Hash getBucketsHash(struct Buckets *buckets) {
    return buckets->hash;
}

static ValueComparator getBucketsValueComparator(struct Buckets *buckets) {
    return buckets->compare;
}

static FreeValueFunction getBucketsFreeValueFunction(struct Buckets *buckets) {
    return buckets->free_value;
}

static struct Buckets *rehashBuckets(struct Buckets* old, uint64_t new_num_of_buckets) {
    struct Buckets *new = newBuckets(
        new_num_of_buckets, old->value_size,
        old->hash, old->compare, old->free_value
    );
    if (!new) return NULL;
    for (uint64_t i = 0; i < old->num_of_buckets; ++i) {
        if (!getListSize(old->buckets[i])) continue;
        struct ListIterator jt;
        setListIteratorNode(&jt, getListHead(old->buckets[i]));
        do {
            if (pushBuckets(new, getListIteratorValue(&jt))) {
                freeBuckets(new, 0);
                return NULL;
            }
            incListIterator(&jt);
        } while (!compareListIteratorNode(&jt, getListHead(old->buckets[i])));
    }
    return new;
}

struct BucketsIterator {
    struct ListIterator it;
    uint64_t index_of_list;
};

static void initBucketsIterator(struct Buckets *buckets, struct BucketsIterator *it) {
    uint64_t i;
    for (i = 0; (i < buckets->num_of_buckets) && !getListSize(buckets->buckets[i]); ++i);
    it->index_of_list = i;
    if (i == buckets->num_of_buckets) return;
    setListIteratorNode(&it->it, getListHead(buckets->buckets[i]));
}

static void incBucketsIterator(struct Buckets *buckets, struct BucketsIterator *it) {
    incListIterator(&it->it);
    if (!compareListIteratorNode(&it->it, getListHead(buckets->buckets[it->index_of_list])))
        return;
    uint64_t i;
    for (
        i = it->index_of_list + 1;
        (i < buckets->num_of_buckets) && !getListSize(buckets->buckets[i]);
        ++i
    );
    it->index_of_list = i;
    if (i == buckets->num_of_buckets) return;
    setListIteratorNode(&it->it, getListHead(buckets->buckets[i]));
}

static uint8_t reachedEndBucketsIterator(struct Buckets *buckets, struct BucketsIterator *it) {
    return it->index_of_list == buckets->num_of_buckets;
}

static void *getBucketsIteratorValue(struct BucketsIterator *it) {
    return getListIteratorValue(&it->it);
}

int initSet(
    Set *set,
    size_t value_size,
    Hash hash,
    ValueComparator compare,
    FreeValueFunction free_value
) {
    set->buckets = newBuckets(INIT_NUM_OF_BUCKETS, value_size, hash, compare, free_value);
    if (!set->buckets) return -1;
    set->size = 0;
    return 0;
}

static uint64_t findClosestBiggerPrime(uint64_t number) {
    if (number <= 2) return 2;
    for (;;++number) {
        uint8_t divider_found = 0;
        for (uint64_t i = 2; i <= (uint64_t)sqrt(number); ++i)
            if (number % i == 0) {
                divider_found = 0;
                break;
            }
        if (!divider_found) break;
    }
    return number;
}

static double loadFactor(Set *set) {
    return (double)set->size / (double)getNumOfBuckets(set->buckets);
}

static int rehash(Set *set) {
    while (loadFactor(set) > MAX_LOAD_FACTOR) {
        uint64_t new_num_of_buckets = findClosestBiggerPrime(
            2 * getNumOfBuckets(set->buckets) + 1
        );
        struct Buckets *new_buckets = rehashBuckets(set->buckets, new_num_of_buckets);
        if (!new_buckets) return -1;
        freeBuckets(set->buckets, 0);
        set->buckets = new_buckets;
    }
    return 0;
}

int pushSet(Set *set, void *value) {
    if (pushBuckets(set->buckets, value)) return -1;
    if (rehash(set)) {
        popBuckets(set->buckets, value, 0);
        return -1;
    }
    return 0;
}

void popSet(Set *set, void *value, uint8_t deep) {
    popBuckets(set->buckets, value, deep);
}

uint8_t containsSet(Set *set, void *value) {
    return containsBuckets(set->buckets, value);
}

uint64_t getSetSize(Set *set) {
    return set->size;
}

void freeSet(Set *set, uint8_t deep) {
    freeBuckets(set->buckets, deep);
}

static int copyElementsSet(Set *dst, Set *src) {
    struct BucketsIterator it;
    for (
        initBucketsIterator(src->buckets, &it);
        !reachedEndBucketsIterator(src->buckets, &it);
        incBucketsIterator(src->buckets, &it)
    )  if (pushBuckets(dst->buckets, getBucketsIteratorValue(&it))) {
        freeSet(dst, 0);
        return -1;
    }
    return 0;
}

static int initSetByExample(Set *new, Set *example) {
    return initSet(
        new,
        getBucketsValueSize(example->buckets),
        getBucketsHash(example->buckets),
        getBucketsValueComparator(example->buckets),
        getBucketsFreeValueFunction(example->buckets)
    );
}

int copySet(Set *dst, Set *src) {
    if (initSetByExample(dst, src) || copyElementsSet(dst, src))
        return -1;
    if (rehash(dst)) {
        freeSet(dst, 0);
        return -2;
    }
    return 0;
}

int unionSet(Set *result, Set *first, Set *second) {
    if (
        initSetByExample(result, first) ||
        copyElementsSet(result, first) ||
        copyElementsSet(result, second)
    ) return -1;
    if (rehash(result)) {
        freeSet(result, 0);
        return -2;
    }
    return 0;
}

int intersectSet(Set *result, Set *first, Set *second) {
    if (initSetByExample(result, first)) return -1;
    struct BucketsIterator it;
    for (
        initBucketsIterator(first->buckets, &it);
        !reachedEndBucketsIterator(first->buckets, &it);
        incBucketsIterator(first->buckets, &it)
    ) if (
        containsSet(second, getBucketsIteratorValue(&it)) &&
        pushBuckets(result->buckets, getBucketsIteratorValue(&it))
    ) goto err;
    if (rehash(result)) goto err;
    return 0;
err:
    freeSet(result, 0);
    return -1;
}

int subtract(Set *difference, Set *minuend, Set *subtrahend) {
    if (copySet(difference, subtrahend)) return -1;
    struct BucketsIterator it;
    for (
        initBucketsIterator(subtrahend->buckets, &it);
        !reachedEndBucketsIterator(subtrahend->buckets, &it);
        incBucketsIterator(subtrahend->buckets, &it)
    ) popBuckets(difference->buckets, getBucketsIteratorValue(&it), 0);
    return 0;
}