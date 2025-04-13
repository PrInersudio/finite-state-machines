#include <iostream>
#include <string>
#include <csignal>
#include <memory>
#include "Memory.hpp"
#include "../Memory/Redis.hpp"

std::unique_ptr<Container> containerPtr = nullptr;

void signalHandler(int signal) {
    std::cout << "\n[!] Сигнал " << signal << " получен. Завершаем работу..." << std::endl;
    setShutdownFlag();
    if (containerPtr) containerPtr.reset();
    std::exit(signal);
}

int main(int argc, char **argv) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации регистра> [--debug]" << std::endl;
        return -1;
    }
    if ((argc > 2) && (std::string(argv[2]) == "--debug")) {
        enableContainerDebug();
        disableOldSetsDeletion();
    }
    struct ShiftRegister reg;
    if (initShiftRegisterFromFile(&reg, argv[1]))
        throw std::runtime_error("Не удалось инициализировать регистр сдвига из файла" + std::string(argv[1]) + ".");
    MinimalShiftRegister min_reg(&reg);
    freeShiftRegister(&reg);
    containerPtr = std::make_unique<Container>();
    getMemoryShiftRegister(min_reg);
}