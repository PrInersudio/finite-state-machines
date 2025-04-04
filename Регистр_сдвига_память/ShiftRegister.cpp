#include "ShiftRegister.hpp"
#include <fstream>

static void readFunction(std::vector<bool> &func, uint64_t size, std::ifstream &file) {
    char ch;
    for (uint64_t i = 0; i < size; ++i) {
        file.get(ch);
        switch (ch) {
            case '1':
                func.push_back(true);
                break;
            case '0':
                func.push_back(false);
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                --i;
                break;
            default:
                file.close();
                throw std::runtime_error("Ошибка при чтении файла. Получен символ" + std::string(1, ch) + ".");
        }
    }
}

ShiftRegister::ShiftRegister(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Не открывается файл " + filename + ".");
    char buf[3];
    file.read(buf, 2);
    this->length = std::stoul(buf);
    if (this->length == 0 || this->length > 32) {
        file.close();
        throw std::runtime_error("Длина регистра должна быть от 0 до 32. Подана " + std::to_string(this->length));
    }
    this->mask = (static_cast<uint32_t>(1) << this->length) - 1;
    readFunction(this->phi, static_cast<uint64_t>(1) << (this->length + 1), file);
    readFunction(this->psi, static_cast<uint64_t>(1) << (this->length + 1), file);
    file.close();
}

uint32_t ShiftRegister::stateFunction(uint32_t state, bool x) {
    state <<= 1;
    return (state | this->phi[state | x]) & this->mask;
}

bool ShiftRegister::outputFunction(uint32_t state, bool x) {
    state <<= 1;
    return this->psi[state | this->phi[state | x]];
}

uint8_t ShiftRegister::getLength() {
    return this->length;
}

uint64_t ShiftRegister::numStates() {
    return static_cast<uint64_t>(1) << this->length;
}