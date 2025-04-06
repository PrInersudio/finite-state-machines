#include "LinearFSM.hpp"
#include <iostream>

bool getInput(uint16_t &input) {
    if (!(std::cin >> input)) {
        std::cout << "Конец." << std::endl;
        return true;
    }
    return false;
}

void printWelcomeMessage() {
    std::cout <<
        "Приветствую. Далее программа принимает любые числа от 0 до 2^16.\n"
        "Вход обрабатывается следующим образом. В случае, когда степень расширения поля\n"
        "равна 1, то число просто берётся по модулю мощности поля. В случае\n"
        "степени расширения большей 1 число будет сначала взято по модулю мощности\n"
        "поля, а потом приведено в систему счисления по основанию характеристики.\n"
        "После чего каждый разряд будет интерпретирован как коэффициент многочлена.\n"
        "Программа завершается, когда полученный вход не является числом.\n"
    << std::endl;
}

bool initState(LinearFSM &lin) {
    std::cout << "Введите начальное состояние из GF(" <<
        *lin.Prime() << (lin.Degree() == 1 ? "" : "^" + std::to_string(lin.Degree())) << ")^"
        << lin.stateLength() << ": ";
    GFMatrix init_state(lin.getGF(), 1, lin.stateLength());
    for (slong i = 0; i < lin.stateLength(); ++i) {
        uint16_t init_state_element;
        if (getInput(init_state_element)) return true;
        init_state(0, i, init_state_element);
    }
    lin.setState(init_state);
    return false;
}

void cycle(LinearFSM &lin) {
    GFMatrix input(lin.getGF(), 1, lin.inputLength());
    while (true) {
        std::cout << "Введите вход из (" <<
            *lin.Prime() << "^" << lin.Degree() << ")^" << lin.inputLength() << ": ";
        for (slong i = 0; i < lin.inputLength(); ++i) {
            uint16_t input_element;
            if (getInput(input_element)) return;
            input(0, i, input_element);
        }
        std::cout << "Выход: " << lin(input) << ". Новое состояние: " << lin.getState() << "." << std::endl;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации линейного автомата>" << std::endl;
        return -1;
    }
    LinearFSM lin = initLinearFSM(argv[1]);
    printWelcomeMessage();
    if (!initState(lin)) cycle(lin);
    return 0;
}