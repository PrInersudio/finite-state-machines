#include <iostream>
#include <string>
#include "Memory.hpp"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации регистра> [--no-clean]" << std::endl;
        return 0;
    }
    if ((argc > 2) && (std::string(argv[2]) == "--no-clean"))
        disableOldSetsDeletion();
    ShiftRegister reg(argv[1]);
    getMemoryShiftRegister(reg, 32);
}