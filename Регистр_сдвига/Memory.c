#include "Memory.h"
#include <stdlib.h>
#include <inttypes.h>

uint64_t hashIOTuple(struct IOTuple *io) {
    uint64_t hash = hashBitArray(io->input_sequence) * 31 + hashBitArray(io->output_sequence);
    return hash;
}

uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2) {
    return compareFirstNBytesOfBitArray(io1->input_sequence, io2->input_sequence, (io1->current_length + 7) / 8) &&
        compareFirstNBytesOfBitArray(io1->output_sequence, io2->output_sequence, (io1->current_length + 7) / 8);
}

int initIOTuple(struct IOTuple *io, uint64_t upper_bound) {
    io->input_sequence = malloc(sizeof(BitArray));
    if (!io->input_sequence)  return -1;
    if(initBitArray(io->input_sequence, upper_bound)) {
        free(io->input_sequence);
        return -2;
    }
    io->output_sequence = malloc(sizeof(BitArray));
    if (!io->output_sequence) {
        free(io->input_sequence);
        return -3;
    }
    if(initBitArray(io->output_sequence, upper_bound)) {
        free(io->output_sequence);
        free(io->input_sequence);
        return -4;
    }
    io->current_length = 0;
    return 0;
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

int copyIOTuple(struct IOTuple *dst, struct IOTuple *src) {
    dst->input_sequence = malloc(sizeof(BitArray));
    if (!dst->input_sequence) return -1;
    if (copyBitArray(dst->input_sequence, src->input_sequence)) {
        free(dst->input_sequence);
        return -2;
    }
    dst->output_sequence = malloc(sizeof(BitArray));
    if (!dst->output_sequence) {
        return -3;
    }
    if (copyBitArray(dst->output_sequence, src->output_sequence)) {
        free(dst->output_sequence);
        freeBitArray(dst->input_sequence);
        free(dst->input_sequence);
        return -4;
    }
    dst->current_length = src->current_length;
    return 0;
}

void printIOTuple(struct IOTuple *io) {
    printf("Вход: ");
    for (uint64_t i = 0; i < io->current_length; ++i)
        printf("%" PRIu8 " ", getBitArrayElement(io->input_sequence, i));
    printf("Выход: ");
    for (uint64_t i = 0; i < io->current_length; ++i)
        printf("%" PRIu8 " ", getBitArrayElement(io->output_sequence, i));
}

void freeIOSets(Set **sets, uint64_t num_of_states, uint8_t deep) {
    for (uint64_t i = 0; i < num_of_states; ++i) {
        freeSet(sets[i], deep);
        free(sets[i]);
    }
    free(sets);
}

Set **newIOSets(uint64_t num_of_states) {
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
    freeIOSets(sets, i, 1);
    return NULL;
}

void printIOSets(Set **sets, uint64_t num_of_states) {
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
    printf("Память автомата равна %" PRIu64 ".\n", memory->memory_size);
    for (uint64_t i = 0; i < 2 * memory->memory_size + 1; ++i) {
        if (memory->memory_table[i]) printIOTuple(memory->memory_table[i]);
        printf("\n");
    }
}

void freeMemory(struct Memory *memory) {
    for (uint64_t i = 0; i < 2 * memory->memory_size + 1; ++i) {
        if (memory->memory_table[i]) {
            freeIOTuple(memory->memory_table[i]);
            free(memory->memory_table[i]);
        }
    }
    free(memory->memory_table);
}

static uint64_t getMemoryTableIndexFromIOTuple(struct IOTuple *io) {
    uint64_t index = 0;
    for (uint64_t i = 0; i < io->current_length; ++i)
        index = (index << 1) | getBitArrayElement(io->input_sequence, i);
    for (uint64_t i = 0; i < io->current_length - 1; ++i)
        index = (index << 1) | getBitArrayElement(io->output_sequence, i);
    return index;
}

int initMemory(
    struct Memory* memory,
    Set **io_sets,
    uint64_t num_of_states,
    uint64_t memory_size,
    uint8_t infinite
) {
    if (infinite) {
        memory->infinite = 1;
        return 0;
    }
    memory->infinite = 0;
    memory->memory_size = memory_size;
    memory->memory_table = malloc(
        ((uint64_t)1 << (2 * memory_size + 1)) * sizeof(struct IOTuple)
    );
    if (!memory->memory_table) return -1;
    for (uint32_t state = 0; state <= (uint32_t)num_of_states - 1; ++state) {
        struct SetIterator it;
        for (
            initSetIterator(io_sets[state], &it);
            !reachedEndSetIterator(&it);
            incSetIterator(&it)
        ) {
            struct IOTuple *io = getSetIteratorValue(&it);
            memory->memory_table[getMemoryTableIndexFromIOTuple(io)] = io;
        }
    }
    return 0;
}