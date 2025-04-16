#include<iostream>
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
        std::cout << "Использование: " << argv[0] << " <файл конфигурации линейного автомата> [--debug]" << std::endl;
        return -1;
    }
    if ((argc > 2) && (std::string(argv[2]) == "--debug")) {
        enableContainerDebug();
        disableOldSetsDeletion();
    }
    LinearFSM lin = initLinearFSM(argv[1]);
    LinearFSM min = lin.minimize(false);
    try {
        lin.numStates();
    } catch(std::overflow_error &e) {
        std::cerr << "Слишком большое количество состояний в автомате" << std::endl;
        exit(-1);
    }
    containerPtr = std::make_unique<Container>();
    getMemoryLinearFSM(min);
    return 0;
}