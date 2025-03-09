#ifndef MEMORY_H
#define MEMORY_H

#include "List.h"
#include "Set.h"

typedef void (*PrintInput)(void *);
typedef void (*PrintOutput)(void *);

struct IOTuple {
    List *input_sequence;
    List *output_sequence;
    size_t input_size;
    size_t output_size;
    Hash hashInput;
    Hash hashOutput;
    ValueComparator compareInputs;
    ValueComparator compareOutputs;
    FreeValueFunction freeInput;
    FreeValueFunction freeOutput;
    PrintValue printInput;
    PrintValue printOutput;
};

uint64_t hashIOTuple(struct IOTuple *io);
uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2);
struct IOTuple *newIOTuple(
    size_t input_size,
    size_t output_size,
    Hash hashInput,
    Hash hashOutput,
    ValueComparator compareInputs,
    ValueComparator compareOutputs,
    FreeValueFunction freeInput,
    FreeValueFunction freeOutput,
    PrintValue printInput,
    PrintValue printOutput
);
void freeIOTuple(struct IOTuple *io);
int pushInputIOTuple(struct IOTuple *io, void *input);
int pushOutputIOTuple(struct IOTuple *io, void *output);
struct IOTuple *copyIOTuple(struct IOTuple *src);
int pushBackInputIOTuple(struct IOTuple *io, void *input);
int pushBackOutputIOTuple(struct IOTuple *io, void *output);

struct Trace {
    struct IOTuple *io;
    void *final_state;
    FreeValueFunction freeState;
};

struct Trace *newTrace(
    size_t input_size,
    size_t output_size,
    Hash hashInput,
    Hash hashOutput,
    ValueComparator compareInputs,
    ValueComparator compareOutputs,
    FreeValueFunction freeInput,
    FreeValueFunction freeOutput,
    PrintValue printInput,
    PrintValue printOutput,
    FreeValueFunction freeState
);
void freeTrace(struct Trace *trace);
void *getTraceFinalState(struct Trace *trace);
struct IOTuple *getTraceIOTuple(struct Trace *trace);
struct IOTuple *popTraceIOTuple(struct Trace *trace);
// 0 - не очищать, 1 - неглубокая очистка, 2 - глубокая очистка.
int setTraceFinalState(struct Trace *trace, void *state, size_t state_size, uint8_t free_prev_mode);
int pushInputTrace(struct Trace *trace, void *input);
int pushOutputTrace(struct Trace *trace, void *output);
struct Trace *copyTrace(struct Trace *src);
int pushBackInputTrace(struct Trace *trace, void *input);
int pushBackOutputTrace(struct Trace *trace, void *output);

List **newTraces(uint64_t num_of_states);
void freeTraces(List **traces, uint64_t num_of_states);

void freeSetsOfIOTuples(Set **sets, uint64_t num_of_states, uint8_t deep);
Set **newSetsOfIOTuples(uint64_t num_of_states);
int checkMemoryCriteria(Set **sets, uint64_t num_of_states);

struct Memory {
    struct IOTuple **memory_table;
    uint64_t memory_table_size;
    uint64_t m;
    uint8_t infinite;
};

void printMemory(struct Memory *memory);
void freeMemory(struct Memory *memory);

#endif