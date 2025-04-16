#include "IOTuple.hpp"
#include <iostream>
#include <sstream>

IOTuple::IOTuple() {}

IOTuple::IOTuple(const GFMatrix &input, const GFMatrix &output) {
    this->push(input, output);
}
IOTuple::IOTuple(const IOTuple& other) : input(other.input), output(other.output) {}

IOTuple::IOTuple(const std::string &str) {
    size_t index = 1;
    while (str[index] != ',') {
        uint64_t num = 0;
        while (std::isdigit(str[index]))
            num = num * 10 + (str[index++] - '0');
        this->input.push_back(num);
        if (str[index] == ':') ++index;
    }
    index += 2;
    while (str[index] != ')') {
        uint64_t num = 0;
        while (std::isdigit(str[index]))
            num = num * 10 + (str[index++] - '0');
        this->output.push_back(num);
        if (str[index] == ':') ++index;
    }
}

void IOTuple::push(const GFMatrix &input, const GFMatrix &output) {
    this->input.emplace_back(input);
    this->output.emplace_back(output);
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
    oss << input[0];
    for (size_t i = 1; i < input.size(); ++i) {
        oss << ":";
        oss << input[i];
    }
    oss << ", ";
    oss << output[0];
    for (size_t i = 1; i < output.size(); ++i) {
        oss << ":";
        oss << output[i];
    }
    oss << ")";
    return oss.str();
}