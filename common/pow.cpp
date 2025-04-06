#include "pow.hpp"
#include <stdexcept>
#include <cmath>

uint64_t pow(uint64_t base, uint64_t exp) {
    if (!base) return 0;
    if (exp * std::ceil(log2(base)) >= 64)
        throw std::overflow_error(
            "Привышена размерность при возведении в степень. "
            "base = " + std::to_string(base) + ", "
            "exp = " + std::to_string(exp) + "."
        );
    uint64_t result = 1;
    while(exp) {
        if (exp % 2 == 0) {
            exp /= 2;
            base *= base;
        } else {
            --exp;
            result *= base;
        }
    }
    return result;
}