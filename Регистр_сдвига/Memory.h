#ifndef MEMORY_H
#define MEMORY_H

#include "List.h"
#include "Set.h"
#include "BitArray.h"

#define MAX_NUM_THREADS 16
#define MAX_MEMORY_TRY (uint64_t)21

struct IOTuple {
    BitArray *input_sequence;
    BitArray *output_sequence;
    uint64_t current_length;
};

uint64_t hashIOTuple(struct IOTuple *io);
uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2);
int initIOTuple(struct IOTuple *io, uint64_t upper_bound);
void freeIOTuple(struct IOTuple *io);
void pushIOTuple(struct IOTuple *io, uint8_t input, uint8_t output);
int copyIOTuple(struct IOTuple *dst, struct IOTuple *src);
void printIOTuple(struct IOTuple *io);

void freeIOSets(Set **sets, uint64_t num_of_states, uint8_t deep);
Set **newIOSets(uint64_t num_of_states);
void printIOSets(Set **sets, uint64_t num_of_states);
int checkMemoryCriteria(Set **sets, uint64_t num_of_states);

#endif