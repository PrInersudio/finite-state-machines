#include <iostream>
#include "LinearFSM.hpp"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации линейного автомата>" << std::endl;
        return -1;
    }
    LinearFSM lin = initLinearFSM(argv[1]);
    LinearFSM min = lin.minimize(1);
    std::cout << "Привидённый вес: " << min.numStates() << std::endl;
    std::cout << "Минимальный " << min << std::endl;
    return 0;
}