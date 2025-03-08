#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include <stdio.h>
#include "BitArray.h"
#include "EquivalenceClass.h"
#include "Graph.h"

struct ShiftRegister {
    uint8_t length;
    BitArray phi;
    BitArray psi;
    uint32_t state;
    uint32_t mask;
};

int initShiftRegisterFromFile(struct ShiftRegister* reg, char* settings_file);
int readState(struct ShiftRegister* reg);
uint8_t useShiftRegister(struct ShiftRegister* reg, uint8_t x);
void freeShiftRegister(struct ShiftRegister* reg);
int shiftRegisterToGraph(struct ShiftRegister *reg, struct Graph *graph);

struct MinimizedShiftRegister {
    List *equivalence_classes;
    uint8_t original_is_minimal;
    uint64_t degree_of_distinguishability;
};

int minimizeShiftRegister(struct MinimizedShiftRegister *minimized, struct ShiftRegister* original);
void freeMinimizedShiftRegister(struct MinimizedShiftRegister *minimized);
void printMinimizedShiftRegister(struct MinimizedShiftRegister *minimized);

#endif