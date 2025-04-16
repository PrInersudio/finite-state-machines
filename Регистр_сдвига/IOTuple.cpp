#include "IOTuple.hpp"
#include <iostream>
#include <sstream>

IOTuple::IOTuple() {}

IOTuple::IOTuple(bool input, bool output) {
    this->push(input, output);
}
IOTuple::IOTuple(const IOTuple& other) : input(other.input), output(other.output) {}

IOTuple::IOTuple(const std::string &str) {
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
    os << io.toString();
    return os;
}

std::string IOTuple::toString() const {
    std::ostringstream oss;
    oss << "(";
    for (bool x : this->input)
        oss << (x ? "1" : "0");
    oss << ", ";
    for (bool y : this->output)
        oss << (y ? "1" : "0");
    oss << ")";
    return oss.str();
}