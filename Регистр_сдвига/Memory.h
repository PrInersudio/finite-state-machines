#ifndef MEMORY_H
#define MEMORY_H

#include "List.h"
#include "Set.h"
#include "BitArray.h"

struct IOTuple {
    BitArray *input_sequence;
    BitArray *output_sequence;
    uint64_t current_length;
};

uint64_t hashIOTuple(struct IOTuple *io);
uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2);
struct IOTuple *newIOTuple(uint64_t upper_bound);
void freeIOTuple(struct IOTuple *io);
void pushIOTuple(struct IOTuple *io, uint8_t input, uint8_t output);
struct IOTuple *copyIOTuple(struct IOTuple *src);
void printIOTuple(struct IOTuple *io);

struct Trace {
    struct IOTuple *io;
    uint32_t final_state;
};

int initTrace(struct Trace *trace, uint64_t upper_bound);
void freeTrace(struct Trace *trace);
uint32_t getTraceFinalState(struct Trace *trace);
struct IOTuple *getTraceIOTuple(struct Trace *trace);
struct IOTuple *popTraceIOTuple(struct Trace *trace);
void setTraceFinalState(struct Trace *trace, uint32_t state);
void updateTrace(struct Trace *trace, uint8_t input, uint8_t output, uint8_t new_state);
int copyTrace(struct Trace *dst, struct Trace *src);

List **newTraces(uint64_t num_of_states);
void freeTraces(List **traces, uint64_t num_of_states);

void freeSetsOfIOTuples(Set **sets, uint64_t num_of_states);
Set **newSetsOfIOTuples(uint64_t num_of_states);
void printSetsOfIOTuples(Set **sets, uint64_t num_of_states);
int checkMemoryCriteria(Set **sets, uint64_t num_of_states);

struct Memory {
    struct IOTuple **memory_table;
    uint64_t memory_table_size;
    uint64_t m;
    uint8_t infinite;
};

void printMemory(struct Memory *memory);
void freeMemory(struct Memory *memory);
uint64_t getMemoryTableIndexFromIOTuple(struct IOTuple *io);

#endif