#include "LinearFSM.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации линейного автомата>" << std::endl;
        return -1;
    }
    LinearFSM lin = initLinearFSM(argv[1]);
    std::cout << "Введите начальное состояние из (" <<
        *lin.Prime() << "^" << lin.Degree() << ")^" << lin.stateLength() << ":";
    GFMatrix init_state(lin.getGF(), 1, lin.stateLength());
    for (slong i = 0; i < lin.stateLength(); ++i) {
        uint16_t input;
        try {
            std::cin >> input;
        } catch (const std::ios_base::failure& e) {
            std::cout << "Конец." << std::endl;
            return 0;
        }
        init_state(0, i, input);
    }
    lin.setState(init_state);
    GFMatrix input(lin.getGF(), 1, lin.inputLength());
    while (true) {
        std::cout << "Введите вход из (" <<
            *lin.Prime() << "^" << lin.Degree() << ")^" << lin.inputLength() << ":";
        for (slong i = 0; i < lin.inputLength(); ++i) {
            uint16_t x;
            try {
                std::cin >> x;
            } catch (const std::ios_base::failure& e) {
                std::cout << "Конец." << std::endl;
                return 0;
            }
            input(0, i, x);
        }
    }
}