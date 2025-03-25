#ifndef MEMORY_H
#define MEMORY_H

#include "List.h"
#include "Set.h"
#include "BitArray.h"

#define MAX_NUM_THREADS 16
#define CACHE_FILE_FORMAT ".SET"
#define СACHE_FILE_NAME_LEN 35

struct IOTuple {
    BitArray *input_sequence;
    BitArray *output_sequence;
    uint64_t current_length;
};

uint64_t hashIOTuple(struct IOTuple *io);
uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2);
int putIOTupleIntoFile(struct IOTuple *io, FILE *fp);
int initIOTuple(struct IOTuple *io, uint64_t upper_bound);
void freeIOTuple(struct IOTuple *io);
void pushIOTuple(struct IOTuple *io, uint8_t input, uint8_t output);
int copyIOTuple(struct IOTuple *dst, struct IOTuple *src);
void printIOTuple(struct IOTuple *io);
int getIOTupleFromFile(struct IOTuple *io, FILE *fp);
void closeCacheFiles(FILE **files, uint64_t num_of_states);
FILE **openCacheFiles(uint64_t num_of_states, uint64_t memory_size, const char *mode);
int getIOSetFromFile(Set *set, FILE *fp, uint64_t memory_size);
int checkMemoryCriteria(uint64_t num_of_states, uint64_t memory_size);
void deleteCacheFiles();

#endif