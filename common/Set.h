#ifndef SET_H
#define SET_H

#include <stdint.h>
#include "List.h"
#include "Hash.h"

#define INIT_NUM_OF_BUCKETS 257
#define MAX_LOAD_FACTOR 4.0

typedef struct {
    void *buckets;
    uint64_t size;
} Set;

int initSet(
    Set *set,
    size_t value_size,
    Hash hash,
    ValueComparator compare,
    FreeValueFunction freeValue,
    PrintValue printValue
);
int pushSet(Set *set, void *value);
void popSet(Set *set, void *value, uint8_t deep);
uint8_t containsSet(Set *set, void *value);
uint64_t getSetSize(Set *set);
void freeSet(Set *set, uint8_t deep);
int copySet(Set *dst, Set *src);
int unionSet(Set *result, Set *first, Set *second);
int intersectSet(Set *result, Set *first, Set *second);
int subtract(Set *difference, Set *minuend, Set *subtrahend);
void printSet(Set *set);

struct BucketsIterator {
    struct ListIterator it;
    uint64_t index_of_list;
};

struct SetIterator {
    struct BucketsIterator it;
    Set *set;
};
void initSetIterator(Set *set, struct SetIterator *it);
uint8_t reachedEndSetIterator(struct SetIterator *it);
void incSetIterator(struct SetIterator *it);
void *getSetIteratorValue(struct SetIterator *it);

#endif