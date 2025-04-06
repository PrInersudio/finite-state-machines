#ifndef LINEARFSM_HPP
#define LINEARFSM_HPP

#include "GF.hpp"
#include <string>

class LinearFSM {
private:
    const GF gf;
    const GFMatrix A, B, C, D;
    GFMatrix state;
public:
    LinearFSM(const GF &gf, const GFMatrix &A, const GFMatrix &B, const GFMatrix &C, const GFMatrix &D, slong n);
    GFMatrix operator()(const GFMatrix &input);
    const GFMatrix &getState() const;
    void setState(const GFMatrix &state);
    const fmpz *Prime() const;
    const slong Degree() const;
    const slong inputLength() const;
    const slong stateLength() const;
    const slong outputLength() const;
    const GF &getGF() const;
};

LinearFSM initLinearFSM(std::string filename);

#endif