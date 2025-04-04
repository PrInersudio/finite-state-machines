#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "ShiftRegister.hpp"

void disableOldSetsDeletion();
void getMemoryShiftRegister(ShiftRegister &reg, uint64_t upper_bound);

#endif