#include "LinearFSM.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

LinearFSM::LinearFSM(const GF &gf, const GFMatrix &A, const GFMatrix &B, const GFMatrix &C, const GFMatrix &D, slong n)
    : gf(gf), A(this->gf, A), B(this->gf, B), C(this->gf, C), D(this->gf, D), state(this->gf, 1, n) {}

GFMatrix LinearFSM::operator()(const GFMatrix &input) {
    GFMatrix output = this->state * C + input * D;
    this->state = this->state * A + input * B;
    return output;
}

const GFMatrix &LinearFSM::getState() const {
    return this->state;
}

void LinearFSM::setState(const GFMatrix &state) {
    this->state = state;
}

const fmpz *LinearFSM::Prime() const {
    return this->gf.Prime();
}
const slong LinearFSM::Degree() const {
    return this->gf.Degree();
}
const slong LinearFSM::inputLength() const {
    return this->B.rows();
}
const slong LinearFSM::stateLength() const {
    return this->A.rows();
}
const slong LinearFSM::outputLength() const {
    return this->D.cols();
}

const GF &LinearFSM::getGF() const {
    return this->gf;
}

static void skipEmptyLines(std::ifstream &file, std::string &line) {
    do std::getline(file, line);
    while (line.find_first_not_of(" \t\n\r") == std::string::npos && !file.eof());
}

static void readMatrix(std::ifstream &file, GFMatrix &mat) {
    std::string line;
    for (slong i = 0; i < mat.rows(); ++i) {
        skipEmptyLines(file, line);
        std::istringstream iss(line);
        for (slong j = 0; j < mat.cols(); ++j) {
            uint16_t n;
            iss >> n;
            mat(i, j, n);
        }
    }
}

LinearFSM initLinearFSM(const std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Не удалось открыть файл " + filename + ".");
    std::string line;
    skipEmptyLines(file, line);
    std::istringstream iss(line);
    uint32_t q;
    iss >> q;
    GF gf(q);
    skipEmptyLines(file, line);
    slong m, n, k;
    iss.clear();
    iss.str(line);
    iss >> m >> n >> k;
    GFMatrix A(gf, n, n), B(gf, m, n), C(gf, n, k), D(gf, m, k);
    for (GFMatrix *mat : { &A, &B, &C, &D })
        readMatrix(file, *mat);
    return LinearFSM(gf, A, B, C, D, n);
}