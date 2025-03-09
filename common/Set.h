#ifndef SET_H
#define SET_H

#include <stdint.h>
#include "List.h"

#define INIT_NUM_OF_BUCKETS 257
#define MAX_LOAD_FACTOR 4.0

typedef uint64_t (*Hash)(void *value);

typedef struct {
    void *buckets;
    uint64_t size;
} Set;

int initSet(
    Set *set,
    size_t value_size,
    Hash hash,
    ValueComparator compare,
    FreeValueFunction free_value
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

#endif