#include <iostream>
#include <string>
#include "Memory.hpp"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации регистра>" << std::endl;
        return -1;
    }
    struct ShiftRegister reg;
    if (initShiftRegisterFromFile(&reg, argv[1]))
        throw std::runtime_error("Не удалось инициализировать регистр сдвига из файла" + std::string(argv[1]) + ".");
    MinimalShiftRegister min_reg(&reg);
    freeShiftRegister(&reg);
    getMemoryShiftRegister(min_reg);
}