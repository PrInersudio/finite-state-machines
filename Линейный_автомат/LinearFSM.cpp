#include "LinearFSM.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "pow.hpp"

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

GFMatrix LinearFSM::stateFunction(const GFMatrix &state, const GFMatrix &input) const {
    return state * A + input * B;
}
GFMatrix LinearFSM::outputFunction(const GFMatrix &state, const GFMatrix &input) const {
    return state * C + input * B;
}

uint64_t LinearFSM::numInputs() const {
    return pow(this->gf.Order(), this->inputLength());
}

uint64_t LinearFSM::numStates() const {
    return pow(this->gf.Order(), this->stateLength());
}

uint64_t LinearFSM::numOutputs() const {
    return pow(this->gf.Order(), this->outputLength());
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

static void copyMtPart(const fq_ctx_t &gf, GFMatrix &Mt, const GFMatrix &BAi, const slong i) {
    fq_mat_t window;
    fq_mat_window_init(window, Mt.raw(),  i * BAi.rows(), 0, (i + 1) * BAi.rows(), BAi.cols(), gf);
    fq_mat_set(window, BAi.raw(), gf);
    fq_mat_window_clear(window, gf);
}

GFMatrix LinearFSM::Mt(const slong t) const {
    GFMatrix Mt(this->gf, t * B.rows(), B.cols());
    GFMatrix BAi = B;
    for (slong i = 0; i < t - 1; ++i) {
        copyMtPart(this->gf.getCTX(), Mt, BAi, t - 1 - i);
        BAi *= A;
    }
    copyMtPart(this->gf.getCTX(), Mt, BAi, 0);
    std::cout << "M_" + std::to_string(t) + ": " << Mt << std::endl;
    return Mt;
}

bool LinearFSM::isStronglyConnected() const {
    return this->Mt(A.minPolyDegree()).rank() == this->stateLength();
}