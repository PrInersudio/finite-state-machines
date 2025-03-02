#include "BitArray.h"
#include <stdlib.h>

int initBitArray(BitArray *array, long long unsigned length) {
    if (!(array->bucket = (uint8_t *)malloc((length + 7) / 8)))
        return -1;
    array->length = length;
    return 0;
}

long long unsigned getBitArrayLength(BitArray *array) {
    return array->length;
}

uint8_t getBitArrayElement(BitArray *array, long long unsigned i) {
    return (array->bucket[i / 8] >> (i % 8)) & (uint8_t)1; 
}

void setBitArrayElement(BitArray *array, long long unsigned i, uint8_t element) {
    if (element) array->bucket[i / 8] |= (uint8_t)1 << (i % 8);
    else array->bucket[i / 8] &= ~((uint8_t)1 << (i % 8));
}

void freeBitArray(BitArray *array) {
    if (array->bucket) {
        free(array->bucket);
        array->bucket = NULL;
    }
    array->length = 0;
}

int readArrayFromFile(BitArray *array, long long unsigned length, FILE *fp) {
    for (long long unsigned i = 0; i < length; ++i) {
        switch (fgetc(fp)) {
            case '1':
                setBitArrayElement(array, i, 1);
                break;
            case '0':
                setBitArrayElement(array, i, 0);
                break;
            case ' ':
            case '\t':
            case '\n':
                --i;
                break;
            default:
                return -1;
        }
    }
    return 0;
}