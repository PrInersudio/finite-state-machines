#include <iostream>
#include "Memory.hpp"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации регистра>" << std::endl;
        return 0;
    }
    ShiftRegister reg(argv[1]);
    getMemoryShiftRegister(reg, 32);
}