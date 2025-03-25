#include "BitArray.h"
#include <stdlib.h>
#include "Hash.h"

int initBitArray(BitArray *array, uint64_t length) {
    if (!(array->bucket = malloc((length + 7) / 8)))
        return -1;
    memset(array->bucket, 0, (length + 7) / 8);
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
        int c = fgetc(fp);
        switch (c) {
            case '1':
                setBitArrayElement(array, i, 1);
                break;
            case '0':
                setBitArrayElement(array, i, 0);
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                --i;
                break;
            default:
                printf("Ошибка при чтении файла. Получен символ %d.\n", c);
                return -1;
        }
    }
    return 0;
}

uint64_t hashBitArray(BitArray *array) {
    return hashBytes(array->bucket, (array->length + 7) / 8);
}

uint8_t compareBitArrays(BitArray *first, BitArray *second) {
    if (first->length != second->length) return 0;
    return !memcmp(first->bucket, second->bucket, (first->length + 7) / 8);
}

uint8_t compareFirstNBytesOfBitArray(BitArray *first, BitArray *second, size_t n) {
    return !memcmp(first->bucket, second->bucket, n);
}

int copyBitArray(BitArray *dst, BitArray *src) {
    if (initBitArray(dst, src->length)) return -1;
    memcpy(dst->bucket, src->bucket, (src->length + 7) / 8);
    return 0;
}