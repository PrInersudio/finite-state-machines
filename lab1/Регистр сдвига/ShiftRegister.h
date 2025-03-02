#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include <stdio.h>
#include "BitArray.h"
#include "List.h"

struct ShiftRegister {
    uint8_t length;
    BitArray phi;
    BitArray psi;
    unsigned state;
    unsigned mask;
};

int initShiftRegisterFromFile(struct ShiftRegister* reg, char* settings_file);
int readState(struct ShiftRegister* reg);
uint8_t useShiftRegister(struct ShiftRegister* reg, uint8_t x);
void freeShiftRegister(struct ShiftRegister* reg);

struct MinimizedShiftRegister {
    List *equivalence_classes;
    uint8_t original_is_minimal;
    long long unsigned degree_of_distinguishability;
};

int minimizeShiftRegister(struct MinimizedShiftRegister *minimized, struct ShiftRegister* original);
void freeMinimizedShiftRegister(struct MinimizedShiftRegister *minimized);
void printMinimizedShiftRegister(struct MinimizedShiftRegister *minimized);

#endif