#ifndef IOTUPLE_HPP
#define IOTUPLE_HPP

#include <string>
#include <vector>
#include "../common/GF.hpp"

class IOTuple {
private:
    std::vector<uint64_t> input;
    std::vector<uint64_t> output;
public:
    IOTuple();
    IOTuple(const GFMatrix &input, const GFMatrix &output);
    IOTuple(const IOTuple& other);
    IOTuple(const std::string &str);
    void push(const GFMatrix &input, const GFMatrix &output);
    bool operator<(const IOTuple &other) const;
    bool operator==(const IOTuple& other) const;
    friend std::ostream &operator<<(std::ostream &os, const IOTuple &io);
    std::string toString() const;
};

#endif