#include "BitArray.h"
#include <stdlib.h>

int initBitArray(BitArray *array, uint64_t length) {
    if (!(array->bucket = malloc((length + 7) / 8 * sizeof(uint8_t))))
        return -1;
    for (uint64_t i = 0; i < (length + 7) / 8; ++i)
        array->bucket[i] = 0;
    array->length = length;
    return 0;
}

uint64_t getBitArrayLength(BitArray *array) {
    return array->length;
}

uint8_t getBitArrayElement(BitArray *array, uint64_t i) {
    return (array->bucket[i / 8] >> (i % 8)) & (uint8_t)1; 
}

void setBitArrayElement(BitArray *array, uint64_t i, uint8_t element) {
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

int readArrayFromFile(BitArray *array, uint64_t length, FILE *fp) {
    for (uint64_t i = 0; i < length; ++i) {
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