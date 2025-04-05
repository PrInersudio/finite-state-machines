#ifndef SHIFT_REGISTER_HPP
#define SHIFT_REGISTER_HPP

#include <vector>
#include <string>
#include <cstdint>

extern "C" {
    #define class ClassVarible // костыль, чтобы обеспечить совместимость c и cpp.
        #include "../Регистр_сдвига/ShiftRegister.h"
    #undef class
}

class DSU {
    private:
        std::vector<std::uint32_t> parent, rank;
    public:
        DSU(size_t size);
        std::uint32_t operator[](std::uint32_t i);
        void operator()(std::uint32_t a, std::uint32_t b);
};

class MinimalShiftRegister {
private:
    DSU equivalence_classes;
    std::vector<bool> phi;
    std::vector<bool> psi;
    uint8_t length;
    std::uint32_t mask;
    uint64_t degree_of_distinguishability;
    uint64_t minimized_weight;

    void copyFunctions(struct ShiftRegister *reg);
    void transformDSUFromEquivalenceClass(List *equivalence_class);
public:
    MinimalShiftRegister(struct ShiftRegister *reg);
    std::uint32_t stateFunction(std::uint32_t state, bool x);
    bool outputFunction(std::uint32_t state, bool x);
    uint8_t getLength() const;
    // Количество значений оригинального регистра.
    uint64_t numStates() const;
    uint64_t getMinimizedWeight() const;
};

#endif