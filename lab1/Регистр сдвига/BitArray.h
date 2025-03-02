#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t *bucket;
    long long unsigned length;
} BitArray;

int initBitArray(BitArray *array, long long unsigned length);
long long unsigned getBitArrayLength(BitArray *array);
uint8_t getBitArrayElement(BitArray *array, long long unsigned i);
void setBitArrayElement(BitArray *array, long long unsigned i, uint8_t element);
void freeBitArray(BitArray *array);
int readArrayFromFile(BitArray *array, long long unsigned length, FILE *fp);

#endif