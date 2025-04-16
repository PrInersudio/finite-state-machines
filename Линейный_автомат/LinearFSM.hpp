#ifndef LINEARFSM_HPP
#define LINEARFSM_HPP

#include "../common/GF.hpp"
#include <string>

class LinearFSM {
public:
    LinearFSM(const GF &gf, const GFMatrix &A, const GFMatrix &B, const GFMatrix &C, const GFMatrix &D, slong n);
    GFMatrix operator()(const GFMatrix &input);
    const GFMatrix &getState() const;
    void setState(const GFMatrix &state);
    const slong inputLength() const;
    const slong stateLength() const;
    const slong outputLength() const;
    const GF &getGF() const;
    GFMatrix stateFunction(const GFMatrix &state, const GFMatrix &input) const;
    GFMatrix outputFunction(const GFMatrix &state, const GFMatrix &input) const;
    uint64_t numInputs() const;
    uint64_t numStates() const;
    uint64_t numOutputs() const;
    bool isStronglyConnected() const;
    LinearFSM minimize(bool print_degree_of_distinguishability) const;
    std::string toString() const;
    class Iterator {
    private:
        GFMatrix mat;
        fq_t one;
        bool end;
    
        friend class LinearFSM;
        Iterator(const GF &gf, slong size, bool end);
        const fq_ctx_t &getCTX() const;
    public:
        Iterator(const Iterator &other);
        ~Iterator();
        Iterator &operator++();
        Iterator operator++(int);
        const GFMatrix &operator*() const;
        bool operator!=(const Iterator &other) const;
    };
    class Range {
        private:
            Iterator b, e;
        public:
            Range(const Iterator &b, const Iterator &e)
                : b(b), e(e) {}
            Iterator begin() const { return b; }
            Iterator end() const { return e; }
    };
    Iterator inputBegin() const;
    Iterator inputEnd() const;
    Iterator stateBegin() const;
    Iterator stateEnd() const;
    Iterator outputBegin() const;
    Iterator outputEnd() const;
    Range inputRange() const;
    Range stateRange() const;
    Range outputRange() const;
    uint64_t getMemoryUpperBound() const;
private:
    const GF gf;
    const GFMatrix A, B, C, D;
    GFMatrix state;

    GFMatrix Mt(const slong t) const;
    GFMatrix buildKt(const slong t) const;
    slong degreeOfDistinguishability(const GFMatrix &Kn) const;
    Range range(Iterator begin, Iterator end) const;
};

LinearFSM initLinearFSM(std::string filename);
std::ostream& operator<<(std::ostream &os, const LinearFSM &lin);

#endif