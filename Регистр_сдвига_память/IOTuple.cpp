#include "IOTuple.hpp"
#include <iostream>

IOTuple::IOTuple() {}

IOTuple::IOTuple(bool input, bool output) {
    this->push(input, output);
}
IOTuple::IOTuple(const IOTuple& other) : input(other.input), output(other.output) {}

IOTuple::IOTuple(std::string str) {
    size_t index = 1;
    while (str[index] != ',')
        this->input.push_back(str[index++] == '1');
    index += 2;
    while (str[index] != ')')
        this->output.push_back(str[index++] == '1');
}

void IOTuple::push(bool input, bool output) {
    this->input.push_back(input);
    this->output.push_back(output);
}

bool IOTuple::operator<(const IOTuple &other) const {
    return std::tie(this->input, this->output) < std::tie(other.input, other.output);
}

bool IOTuple::operator==(const IOTuple& other) const {
    return this->input == other.input && this->output == other.output;
}

std::ostream &operator<<(std::ostream &os, const IOTuple &io) {
    os << io.to_string();
    return os;
}

std::string IOTuple::to_string() const {
    std::string str = "(";
    for (bool x : this->input) str += (x ? "1" : "0");
    str += ", ";
    for (bool y : this->output) str += (y ? "1" : "0");
    str += ")";
    return str;
}

namespace std {
    string to_string(const IOTuple& io) {
        return io.to_string();
    }
}