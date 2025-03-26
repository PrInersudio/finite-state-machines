#ifndef SHIFT_REGISTER_HPP
#define SHIFT_REGISTER_HPP

#include <vector>
#include <string>
#include <cstdint>

class ShiftRegister {
private:
    std::vector<bool> phi;
    std::vector<bool> psi;
    uint8_t length;
    uint32_t mask;
public:
    ShiftRegister(std::string filename);
    uint32_t stateFunction(uint32_t state, bool x);
    bool outputFunction(uint32_t state, bool x);
    uint8_t getLength();
};

#endif