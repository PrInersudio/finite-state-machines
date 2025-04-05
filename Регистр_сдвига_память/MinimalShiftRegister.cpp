#include "MinimalShiftRegister.hpp"
#include <fstream>

DSU::DSU(size_t size) : parent(size), rank(size, 1) {
    for (std::uint32_t i = 0; i <= static_cast<std::uint32_t>(size - 1); ++i) this->parent[i] = i;
}

std::uint32_t DSU::operator[](std::uint32_t i) {
    if (this->parent[i] != i) this->parent[i] = (*this)[this->parent[i]];
    return this->parent[i];
}

void DSU::operator()(std::uint32_t a, std::uint32_t b) {
    a = (*this)[a];
    b = (*this)[b];
    if (a == b) return;
    if (this->rank[a] < this->rank[b]) std::swap(a, b);
    this->parent[b] = a;
    if (this->rank[a] == this->rank[b]) ++rank[a];
}

void MinimalShiftRegister::transformDSUFromEquivalenceClass(List *equivalence_class) {
    if (!getListSize(equivalence_class)) return;
    struct ListIterator it;
    setListIteratorNode(&it, getListHead(equivalence_class));
    uint32_t class_leader = *static_cast<uint32_t *>(getListIteratorValue(&it));
    do {
        this->equivalence_classes(class_leader, *static_cast<uint32_t *>(getListIteratorValue(&it)));
        incListIterator(&it);
    } while (!compareListIteratorNode(&it, getListHead(equivalence_class)));
}

void MinimalShiftRegister::copyFunctions(struct ShiftRegister *reg) {
    uint64_t funcs_size = static_cast<uint64_t>(1) << (reg->length + 1);
    this->phi.resize(funcs_size);
    this->psi.resize(funcs_size);
    for (uint64_t i = 0; i < funcs_size; ++i) {
        this->phi[i] = static_cast<bool>(getBitArrayElement(&reg->phi, i));
        this->psi[i] = static_cast<bool>(getBitArrayElement(&reg->psi, i));
    }
}

MinimalShiftRegister::MinimalShiftRegister(struct ShiftRegister *reg)
    : equivalence_classes(static_cast<uint64_t>(1) << reg->length),
    length(reg->length),  mask(reg->mask) {
    struct Minimized minimized;
    if (minimizeShiftRegister(&minimized, reg))
        throw std::runtime_error("Ошибка при попытке минимизации регистра сдвига.");
    if (!getListSize(minimized.equivalence_classes))
        throw std::runtime_error("Список классов эквивалентности пустой."); 
    struct ListIterator it;
    setListIteratorNode(&it, getListHead(minimized.equivalence_classes));
    do {
        this->transformDSUFromEquivalenceClass(static_cast<List *>(getListIteratorValue(&it)));
        incListIterator(&it);
    } while (!compareListIteratorNode(&it, getListHead(minimized.equivalence_classes)));
    this->degree_of_distinguishability = minimized.degree_of_distinguishability;
    this->minimized_weight = getListSize(minimized.equivalence_classes);
    freeMinimized(&minimized);
    this->copyFunctions(reg);
}

std::uint32_t MinimalShiftRegister::stateFunction(std::uint32_t state, bool x) {
    state <<= 1;
    return this->equivalence_classes[(state | phi[state | x]) & mask];
}

bool MinimalShiftRegister::outputFunction(std::uint32_t state, bool x) {
    state <<= 1;
    return psi[state | phi[state | x]];
}

uint8_t MinimalShiftRegister::getLength() const {
    return length;
}

uint64_t MinimalShiftRegister::numStates() const {
    return static_cast<uint64_t>(1) << length;
}

uint64_t MinimalShiftRegister::getMinimizedWeight() const {
    return this->minimized_weight;
}