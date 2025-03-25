#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include "BitArray.h"
#include "Graph.h"
#include "Minimized.h"
#include "Memory.h"

struct ShiftRegister {
    uint8_t length;
    BitArray phi;
    BitArray psi;
    uint32_t state;
    uint32_t mask;
};

int initShiftRegisterFromFile(struct ShiftRegister* reg, char* settings_file);
int readState(struct ShiftRegister* reg);
uint32_t getState(struct ShiftRegister* reg);
uint8_t useShiftRegister(struct ShiftRegister* reg, uint8_t x);
void freeShiftRegister(struct ShiftRegister* reg);
int shiftRegisterToGraph(struct ShiftRegister *reg, struct Graph *graph);
int minimizeShiftRegister(struct Minimized *minimized, struct ShiftRegister* original);
void printState(uint32_t *state);
int getMemoryShiftRegister(struct ShiftRegister *reg, uint64_t *memory_size);

#endif