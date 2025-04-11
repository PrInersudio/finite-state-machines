#include "LinearFSM.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
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

GFMatrix LinearFSM::Mt(const slong t) const {
    GFMatrix Mt(this->gf, t * B.rows(), B.cols());
    GFMatrix BAi = B;
    for (slong i = 0; i < t - 1; ++i) {
        GFSubmatrix sub(Mt, (t - 1 - i) * BAi.rows(), 0, (t - i) * BAi.rows(), BAi.cols());
        sub = BAi;
        BAi *= A;
    }
    GFSubmatrix sub(Mt,  0, 0, BAi.rows(), BAi.cols());
    sub = BAi;
    std::cout << "M_" + std::to_string(t) + ": " << Mt << std::endl;
    return Mt;
}

bool LinearFSM::isStronglyConnected() const {
    return this->Mt(A.minPolyDegree()).rank() == this->stateLength();
}

GFMatrix LinearFSM::buildKt(const slong t) const {
    GFMatrix Kt(this->gf, C.rows(), t * C.cols());
    GFMatrix AiC = C;
    for (slong i = 0; i < t - 1; ++i) {
        GFSubmatrix sub(Kt, 0, i * AiC.cols(), AiC.rows(), (i + 1) * AiC.cols());
        sub = AiC;
        AiC = A * AiC;
    }
    GFSubmatrix sub(Kt, 0, (t - 1) * AiC.cols(), AiC.rows(), t * AiC.cols());
    sub = AiC;
    std::cout << "K_" + std::to_string(t) + ": " << Kt << std::endl;
    return Kt;
}

slong LinearFSM::degreeOfDistinguishability(const GFMatrix &Kn) const {
    slong degree;
    slong rank = 0;
    for (degree = 0; degree < this->stateLength(); ++degree) {
        fq_mat_t window;
        fq_mat_window_init(window, Kn.raw(), 0, 0, C.rows(), (degree + 1) * C.cols(), this->gf.getCTX());
        slong new_rank = fq_mat_rank(window, this->gf.getCTX());
        fq_mat_window_clear(window, this->gf.getCTX());
        if (rank == new_rank) break;
        rank = new_rank;
    }
    return degree;
}

static GFMatrix getT(const GFMatrix &Kn) {
    GFMatrix reducedKn(Kn.getGF(), Kn.rows(), Kn.cols());
    Kn.reduce(reducedKn);
    std::vector<slong> basis_indexes;
    slong j = 0;
    for (slong i = 0; i < reducedKn.rows(); ++i) {
        while (j < Kn.cols() &&
            fq_is_zero(fq_mat_entry(reducedKn.raw(), i, j),
                reducedKn.getGF().getCTX())) ++j;
        if (j >= Kn.cols()) break;
        basis_indexes.push_back(j);
        ++j;
    }
    GFMatrix T(Kn.getGF(), Kn.rows(), static_cast<slong>(basis_indexes.size()));
    for (slong i = 0; i < T.cols(); ++i) {
        GFSubmatrix subT(T, 0, i, T.rows(), i + 1);
        GFSubmatrix subKn(Kn, 0, basis_indexes[i], Kn.rows(), basis_indexes[i] + 1);
        subT = subKn;
    }
    std::cout << "T: " << T << std::endl;
    return T;
}

static GFMatrix getU(const GFMatrix &T) {
    GFMatrix KT = GFMatrix(T.getGF(), T.rows(), T.cols()).one();
    GFMatrix T_transposed = T.transpose();
    // https://vlab.fandom.com/ru/wiki/Псевдообратные_матрицы
    // Если столбцы матрицы A линейно независимы, тогда матрица AᵀA обратима.
    // В таком случае псевдообратная матрица задаётся формулой A+ = (AᵀA)^-1 * Aᵀ
    GFMatrix T_pinv = ((T_transposed * T)^-1) * T_transposed;
    GFMatrix U = KT * T_pinv;
    std::cout << "U: " << U << std::endl;
    return U;
}

LinearFSM LinearFSM::minimize(bool print_degree_of_distinguishability) const {
    GFMatrix Kn = this->buildKt(this->stateLength());
    if (print_degree_of_distinguishability)
        std::cout << "Степень различимости: " << this->degreeOfDistinguishability(Kn) << std::endl;
    GFMatrix T = getT(Kn);
    if (T.cols() == T.rows()) {
        std::cout << "Матрица T квадратная. Следовательно автомат уже минимален." << std::endl;
        return *this;
    }
    std::cout << "Матрица T не квадратная. Следовательно автомат не минимален." << std::endl;
    GFMatrix U = getU(T);
    GFSubmatrix F(U, 0, 0, T.cols(), U.cols());
    std::cout << "F: " << F << std::endl;
    GFMatrix new_A = F * A * T;
    return LinearFSM(this->gf, new_A, B * T, F * C, D, new_A.rows());
}

std::string LinearFSM::toString() const {
    std::ostringstream ss;
    ss  << "Линейный автомат: " << std::endl
        << "Поле: " + this->gf.toString() << std::endl
        << "A: " << A << std::endl
        << "B: " << B << std::endl
        << "C: " << C << std::endl
        << "D: " << D << std::endl;
    return ss.str();
}

std::ostream& operator<<(std::ostream &os, const LinearFSM &lin) {
    os << lin.toString();
    return os;
}