#include "Memory.h"
#include <stdlib.h>
#include <inttypes.h>

uint64_t hashIOTuple(struct IOTuple *io) {
    return
        (
            io->hashInput ?
            deepHashList(io->input_sequence, io->hashInput) :
            hashList(io->input_sequence)
        ) * 31 +
        (
            io->hashOutput ?
            deepHashList(io->output_sequence, io->hashOutput) :
            hashList(io->output_sequence)
        );
}

uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2) {
    uint8_t rc = 
        (
            io1->compareInputs ?
            deepCompareLists(
                io1->input_sequence,
                io2->input_sequence,
                io1->compareInputs
            ) :
            compareLists(io1->input_sequence, io2->input_sequence)
        ) &&
        (
            io1->compareOutputs ?
            deepCompareLists(
                io1->output_sequence,
                io2->output_sequence,
                io1->compareOutputs
            ) :
            compareLists(io1->output_sequence, io2->output_sequence)
        );
    return rc;
}

struct IOTuple *newIOTuple(
    size_t input_size, size_t output_size,
    Hash hashInput, Hash hashOutput,
    ValueComparator compareInputs, ValueComparator compareOutputs,
    FreeValueFunction freeInput, FreeValueFunction freeOutput,
    PrintValue printInput, PrintValue printOutput
) {
    struct IOTuple *io = malloc(sizeof(struct IOTuple));
    if (!io) return NULL;
    io->input_sequence = malloc(sizeof(List));
    if (!io->input_sequence) {
        free(io);
        return NULL;
    }
    initList(io->input_sequence);
    io->output_sequence = malloc(sizeof(List));
    if (!io->output_sequence) {
        free(io->input_sequence);
        free(io);
        return NULL;
    }
    initList(io->output_sequence);
    io->input_size = input_size;
    io->output_size = output_size;
    io->hashInput = hashInput;
    io->hashOutput =hashOutput;
    io->compareInputs = compareInputs;
    io->compareOutputs = compareOutputs;
    io->freeInput = freeInput;
    io->freeOutput = freeOutput;
    io->printInput = printInput;
    io->printOutput = printOutput;
    return io;
}

void freeIOTuple(struct IOTuple *io) {
    if (!io->freeInput) clearList(io->input_sequence);
    else deepClearList(io->input_sequence, io->freeInput);
    free(io->input_sequence);
    if (!io->freeOutput) clearList(io->output_sequence);
    else deepClearList(io->output_sequence, io->freeOutput);
    free(io->output_sequence);
}

int pushInputIOTuple(struct IOTuple *io, void *input) {
    return pushForwardList(io->input_sequence, input, io->input_size);
}

int pushOutputIOTuple(struct IOTuple *io, void *output) {
    return pushForwardList(io->output_sequence, output, io->output_size);
}

int pushBackInputIOTuple(struct IOTuple *io, void *input) {
    return pushList(io->input_sequence, input, io->input_size);
}

int pushBackOutputIOTuple(struct IOTuple *io, void *output) {
    return pushList(io->output_sequence, output, io->output_size);
}

struct IOTuple *copyIOTuple(struct IOTuple *src) {
    struct IOTuple *dst = malloc(sizeof(struct IOTuple));
    if (!dst) return NULL;
    dst->input_sequence = malloc(sizeof(List));
    if (!dst->input_sequence) {
        free(dst);
        return NULL;
    }
    if (copyList(dst->input_sequence, src->input_sequence)) {
        free(dst->input_sequence);
        free(dst);
        return NULL;
    }
    dst->output_sequence = malloc(sizeof(List));
    if (!dst->output_sequence) {
        clearList(dst->input_sequence);
        free(dst->input_sequence);
        free(dst);
        return NULL;
    }
    if (copyList(dst->output_sequence, src->output_sequence)) {
        free(dst->output_sequence);
        clearList(dst->input_sequence);
        free(dst->input_sequence);
        free(dst);
        return NULL;
    }
    dst->input_size = src->input_size;
    dst->output_size = src->output_size;
    dst->compareInputs = src->compareInputs;
    dst->compareOutputs = src->compareOutputs;
    dst->freeInput = src->freeInput;
    dst->freeOutput = src->freeOutput;
    dst->printInput = src->printInput;
    dst->printOutput = src->printOutput;
    dst->hashInput = src->hashInput;
    dst->hashOutput = src->hashOutput;
    return dst;
}

List *getIOTupleInputSequence(struct IOTuple *io) {
    return io->input_sequence;
}

void printIOTuple(struct IOTuple *io) {
    printf("Вход: ");
    printList(io->input_sequence, io->printInput);
    printf("Выход: ");
    printList(io->output_sequence, io->printOutput);
}

int initTrace(
    struct Trace *trace,
    size_t input_size,
    size_t output_size,
    Hash hashInput,
    Hash hashOutput,
    ValueComparator compareInputs,
    ValueComparator compareOutputs,
    FreeValueFunction freeInput,
    FreeValueFunction freeOutput,
    PrintInput printInput,
    PrintInput printOutput,
    FreeValueFunction freeState
) {
    trace->io = newIOTuple(
        input_size, output_size,
        hashInput, hashOutput,
        compareInputs, compareOutputs,
        freeInput, freeOutput,
        printInput, printOutput
    );
    if (!trace->io) return -1;
    trace->final_state = NULL;
    trace->freeState = freeState;
    trace->state_size = 0;
    return 0;
}

void freeTrace(struct Trace *trace) {
    if (trace->io) {
        freeIOTuple(trace->io);
        free(trace->io);
    }
    if (trace->final_state) {
        if (trace->freeState) trace->freeState(trace->final_state); 
        else free(trace->final_state);
    }
}

void *getTraceFinalState(struct Trace *trace) {
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

int setTraceFinalState(struct Trace *trace, void *state, size_t state_size, uint8_t free_prev_mode) {
    void *new_state = malloc(state_size);
    if (!new_state) return -1;
    memcpy(new_state, state, state_size);
    if (trace->final_state)
        switch (free_prev_mode) {
        case 1:
            free(trace->final_state);
            break;
        case 2:
            trace->freeState(trace->final_state);
            free(trace->final_state);
            break;
        default:
            break;
        }
    trace->final_state = new_state;
    trace->state_size = state_size;
    return 0;
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

int pushInputTrace(struct Trace *trace, void *input) {
    return pushInputIOTuple(trace->io, input);
}

int pushOutputTrace(struct Trace *trace, void *output) {
    return pushOutputIOTuple(trace->io, output);
}

int pushBackInputTrace(struct Trace *trace, void *input) {
    return pushBackInputIOTuple(trace->io, input);
}

int pushBackOutputTrace(struct Trace *trace, void *output) {
    return pushBackOutputIOTuple(trace->io, output);
}

int copyTrace(struct Trace *dst, struct Trace *src) {
    dst->io = copyIOTuple(src->io);
    if (!dst->io) return -1;
    dst->final_state = malloc(src->state_size);
    if (!dst->final_state) {
        freeIOTuple(dst->io);
        return -2;
    }
    memcpy(dst->final_state, src->final_state, src->state_size);
    dst->freeState = src->freeState;
    dst->state_size = src->state_size;
    return 0;
}

void freeSetsOfIOTuples(Set **sets, uint64_t num_of_states, uint8_t deep) {
    for (uint64_t i = 0; i < num_of_states; ++i) {
        freeSet(sets[i], deep);
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
    freeSetsOfIOTuples(sets, i, 0);
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