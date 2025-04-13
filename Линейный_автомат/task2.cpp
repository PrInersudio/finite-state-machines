#include <iostream>
#include "LinearFSM.hpp"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации линейного автомата>" << std::endl;
        return -1;
    }
    LinearFSM lin = initLinearFSM(argv[1]);
    LinearFSM min = lin.minimize(1);
    try {
        std::cout << "Привидённый вес: " << min.numStates() << std::endl;
    } catch (std::overflow_error &e) {
        std::cout << "Привидённый вес: " << min.getGF().Order() << "^" << min.stateLength() << std::endl;
    }
    std::cout << "Минимальный " << min << std::endl;
    return 0;
}