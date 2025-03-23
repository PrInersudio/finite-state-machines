#include "Memory.h"
#include <stdlib.h>
#include <inttypes.h>

uint64_t hashIOTuple(struct IOTuple *io) {
    uint64_t hash = hashBitArray(io->input_sequence) * 31 + hashBitArray(io->output_sequence);
    return hash;
}

uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2) {
    return compareBitArrays(io1->input_sequence, io2->input_sequence) &&
        compareBitArrays(io1->output_sequence, io2->output_sequence);
}

struct IOTuple *newIOTuple(uint64_t upper_bound) {
    struct IOTuple *io = malloc(sizeof(struct IOTuple));
    if (!io) return NULL;
    io->input_sequence = malloc(sizeof(BitArray));
    if (!io->input_sequence) {
        free(io);
        return NULL;
    }
    if(initBitArray(io->input_sequence, upper_bound)) {
        free(io->input_sequence);
        free(io);
        return NULL;
    }
    io->output_sequence = malloc(sizeof(BitArray));
    if (!io->output_sequence) {
        free(io->input_sequence);
        free(io);
        return NULL;
    }
    if(initBitArray(io->output_sequence, upper_bound)) {
        free(io->output_sequence);
        free(io->input_sequence);
        free(io);
        return NULL;
    }
    io->current_length = 0;
    return io;
}

void freeIOTuple(struct IOTuple *io) {
    freeBitArray(io->input_sequence);
    free(io->input_sequence);
    freeBitArray(io->output_sequence);
    free(io->output_sequence);
}

void pushIOTuple(struct IOTuple *io, uint8_t input, uint8_t output) {
    setBitArrayElement(io->input_sequence, io->current_length, input);
    setBitArrayElement(io->output_sequence, io->current_length, output);
    ++io->current_length;
}

struct IOTuple *copyIOTuple(struct IOTuple *src) {
    struct IOTuple *dst = malloc(sizeof(struct IOTuple));
    if (!dst) return NULL;
    dst->input_sequence = malloc(sizeof(BitArray));
    if (!dst->input_sequence) {
        free(dst);
        return NULL;
    }
    if (copyBitArray(dst->input_sequence, src->input_sequence)) {
        free(dst->input_sequence);
        free(dst);
        return NULL;
    }
    dst->output_sequence = malloc(sizeof(BitArray));
    if (!dst->output_sequence) {
        free(dst);
        return NULL;
    }
    if (copyBitArray(dst->output_sequence, src->output_sequence)) {
        free(dst->output_sequence);
        freeBitArray(dst->input_sequence);
        free(dst->input_sequence);
        free(dst);
        return NULL;
    }
    dst->current_length = src->current_length;
    return dst;
}

void printIOTuple(struct IOTuple *io) {
    printf("Вход: ");
    for (uint64_t i = 0; i < io->current_length; ++i)
        printf("%" PRIu8 " ", getBitArrayElement(io->input_sequence, i));
    printf("Выход: ");
    for (uint64_t i = 0; i < io->current_length; ++i)
        printf("%" PRIu8 " ", getBitArrayElement(io->output_sequence, i));
}

int initTrace(struct Trace *trace, uint64_t upper_bound) {
    trace->io = newIOTuple(upper_bound);
    if (!trace->io) return -1;
    trace->final_state = 0;
    return 0;
}

void freeTrace(struct Trace *trace) {
    if (trace->io) {
        freeIOTuple(trace->io);
        free(trace->io);
    }
}

uint32_t getTraceFinalState(struct Trace *trace) {
    return trace->final_state;
}

struct IOTuple *getTraceIOTuple(struct Trace *trace) {
    return trace->io;
}

struct IOTuple *popTraceIOTuple(struct Trace *trace) {
    struct IOTuple *io = trace->io;
    trace->io = NULL;
    return io;
}

void setTraceFinalState(struct Trace *trace, uint32_t state) {
    trace->final_state = state;
}

List **newTraces(uint64_t num_of_states) {
    List **traces = malloc(num_of_states * sizeof(List *));
    if (!traces) return NULL;
    for (uint64_t i = 0; i < num_of_states; ++i) {
        traces[i] = malloc(sizeof(List));
        if (!traces[i]) {
            freeTraces(traces, i);
            return NULL;
        }
        initList(traces[i]);
    }
    return traces;
}

void freeTraces(List **traces, uint64_t num_of_states) {
    for (uint64_t i = 0; i < num_of_states; ++i) {
        deepClearList(traces[i], (FreeValueFunction)freeTrace);
        free(traces[i]);
    }
    free(traces);
}

void updateTrace(struct Trace *trace, uint8_t input, uint8_t output, uint8_t new_state) {
    pushIOTuple(trace->io, input, output);
    trace->final_state = new_state;
}

int copyTrace(struct Trace *dst, struct Trace *src) {
    if (!dst) return -1;
    dst->io = copyIOTuple(src->io);
    if (!dst->io) return -2;
    dst->final_state = src->final_state;
    return 0;
}

void freeSetsOfIOTuples(Set **sets, uint64_t num_of_states) {
    for (uint64_t i = 0; i < num_of_states; ++i) {
        freeSet(sets[i], 0);
        free(sets[i]);
    }
    free(sets);
}

Set **newSetsOfIOTuples(uint64_t num_of_states) {
    Set **sets = malloc(num_of_states * sizeof(Set *));
    if (!sets) return NULL;
    uint64_t i;
    for (i = 0; i < num_of_states; ++i) {
        Set *set = malloc(sizeof(Set));
        if (!set) goto err;
        if (initSet(
                set, sizeof(struct IOTuple),
                (Hash)hashIOTuple, (ValueComparator)compareIOTuples, 
                (FreeValueFunction)freeIOTuple, (PrintValue)printIOTuple
        )) {
            free(set);
            goto err;
        }
        sets[i] = set;
    }
    return sets;
err:
    freeSetsOfIOTuples(sets, i);
    return NULL;
}

void printSetsOfIOTuples(Set **sets, uint64_t num_of_states) {
    for (uint64_t i = 0; i < num_of_states; ++i) {
        printf("%" PRIu64 ": ", i);
        printSet(sets[i]);
        printf("\n");
    }
    printf("\n");
}

int checkMemoryCriteria(Set **sets, uint64_t num_of_states) {
    for (uint64_t i = 0; i < num_of_states - 1; ++i)
        for (uint64_t j = i + 1; j < num_of_states; ++j) {
            Set intersection;
            if (intersectSet(&intersection, sets[i], sets[j]))
                return -1;
            uint8_t intersects = getSetSize(&intersection) != 0;
            freeSet(&intersection, 0);
            if (intersects) return 0;
        }
    return 1;
}

void printMemory(struct Memory *memory) {
    if (memory->infinite) {
        printf("Память автомата бесконечна.\n");
        return;
    }
    printf("Память автомата равна %" PRIu64 ".\n", memory->m);
    for (uint64_t i = 0; i < memory->memory_table_size; ++i) {
        if (memory->memory_table[i]) printIOTuple(memory->memory_table[i]);
        printf("\n");
    }
}

void freeMemory(struct Memory *memory) {
    for (uint64_t i = 0; i < memory->memory_table_size; ++i) {
        if (memory->memory_table[i]) {
            freeIOTuple(memory->memory_table[i]);
            free(memory->memory_table[i]);
        }
    }
    free(memory->memory_table);
}

uint64_t getMemoryTableIndexFromIOTuple(struct IOTuple *io) {
    uint64_t index = 0;
    for (uint64_t i = 0; i < io->current_length; ++i)
        index = (index << 1) | getBitArrayElement(io->input_sequence, i);
    for (uint64_t i = 0; i < io->current_length - 1; ++i)
        index = (index << 1) | getBitArrayElement(io->output_sequence, i);
    return index;
}