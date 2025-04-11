#ifndef LINEARFSM_HPP
#define LINEARFSM_HPP

#include "GF.hpp"
#include <string>

class LinearFSM {
private:
    const GF gf;
    const GFMatrix A, B, C, D;
    GFMatrix state;

    GFMatrix Mt(const slong t) const;
    GFMatrix buildKt(const slong t) const;
    slong degreeOfDistinguishability(const GFMatrix &Kn) const;
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
};

LinearFSM initLinearFSM(std::string filename);
std::ostream& operator<<(std::ostream &os, const LinearFSM &lin);

#endif