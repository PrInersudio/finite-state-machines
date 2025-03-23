#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t *bucket;
    uint64_t length;
} BitArray;

int initBitArray(BitArray *array, uint64_t length);
uint64_t getBitArrayLength(BitArray *array);
uint8_t getBitArrayElement(BitArray *array, uint64_t i);
void setBitArrayElement(BitArray *array, uint64_t i, uint8_t element);
void freeBitArray(BitArray *array);
int readArrayFromFile(BitArray *array, uint64_t length, FILE *fp);
uint64_t hashBitArray(BitArray *array);
uint8_t compareBitArrays(BitArray *first, BitArray *second);
int copyBitArray(BitArray *dst, BitArray *src);

#endif