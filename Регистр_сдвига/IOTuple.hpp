#ifndef IOTUPLE_HPP
#define IOTUPLE_HPP

#include <string>
#include <vector>

class IOTuple {
private:
    std::vector<bool> input;
    std::vector<bool> output;
public:
    IOTuple();
    IOTuple(bool input, bool output);
    IOTuple(const IOTuple& other);
    IOTuple(const std::string &str);
    void push(bool input, bool output);
    bool operator<(const IOTuple &other) const;
    bool operator==(const IOTuple& other) const;
    friend std::ostream &operator<<(std::ostream &os, const IOTuple &io);
    std::string toString() const;
};

#endif