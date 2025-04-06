#include "GF.hpp"
#include <flint/fmpz_factor.h>
#include <flint/fmpz.h>
#include <stdexcept>
#include <iostream>

GF::GF(uint32_t q) : q(q) {
    if (q < 2 || q > (static_cast<std::uint32_t>(1) << 16))
        throw std::out_of_range("Мощность поля должна быть не менее 2 и не более 2^16. Подана " +
            std::to_string(q) + ".");
    fmpz_factor_t factor;
    fmpz_factor_init(factor);
    fmpz_t qf;
    fmpz_init_set_ui(qf, q);
    fmpz_factor(factor, qf);
    fmpz_clear(qf);
    if (factor->num != 1) {
        fmpz_factor_clear(factor);
        throw std::invalid_argument("Мощность поля должна быть степенью простого числа.");
    }
    fq_ctx_init(this->ctx, &factor->p[0], factor->exp[0], "x");
    fmpz_factor_clear(factor);
    std::cout << "Из полученного ввода выведены следующие параметры поля:" << std::endl;
    fq_ctx_print(this->ctx);
    std::cout << std::endl;
}

const fmpz *GF::Prime() const {
    return fq_ctx_prime(this->ctx);
}

slong GF::Degree() const {
    return fq_ctx_degree(this->ctx);
}

GF::GF(const GF &other) : q(other.q) {
    fq_ctx_init(this->ctx, other.Prime(), other.Degree(), "x");
}

GF::~GF() {
    fq_ctx_clear(this->ctx);
}

void GF::getPoly(fmpz_poly_t f, std::uint16_t n) const {
    n %= q;
    fmpz_poly_init(f);
    slong i = 0;
    do fmpz_poly_set_coeff_ui(f, i++, n % *this->Prime());
    while (n /= *this->Prime());
}

GFElement GF::operator()(std::uint16_t n) const {
    fmpz_poly_t f;
    this->getPoly(f, n);
    GFElement element(*this, f);
    fmpz_poly_clear(f);
    return element;
}

const fq_ctx_t &GF::getCTX() const {
    return this->ctx;
}

GFElement::GFElement(const GF &gf, const fmpz_poly_t f) : gf(gf) {
    fq_init(this->element, this->gf.getCTX());
    fq_set_fmpz_poly(this->element, f, this->gf.getCTX());
}

GFElement::GFElement(const GFElement &other) : gf(other.gf) {
    fq_init(this->element, this->gf.getCTX());
    fq_set(this->element, other.element, this->gf.getCTX());
}

GFElement::~GFElement() {
    fq_clear(this->element, this->gf.getCTX());
}

const fq_t &GFElement::raw() const {
    return this->element;
}

GFElement &GFElement::operator+=(const GFElement &other) {
    fq_add(this->element, this->element, other.element, this->gf.getCTX());
    return *this;
}

GFElement &GFElement::operator-=(const GFElement &other) {
    fq_sub(this->element, this->element, other.element, this->gf.getCTX());
    return *this;
}

GFElement &GFElement::operator*=(const GFElement &other) {
    fq_mul(this->element, this->element, other.element, this->gf.getCTX());
    return *this;
}

GFElement &GFElement::operator/=(const GFElement &other) {
    fq_div(this->element, this->element, other.element, this->gf.getCTX());
    return *this;
}

void GFElement::inv() {
    fq_inv(this->element, this->element, this->gf.getCTX());
}

GFElement &GFElement::operator^=(int32_t e) {
    if (e < 0) {
        this->inv();
        e = -e;
    }
    fq_pow_ui(this->element, this->element, e, this->gf.getCTX());
    return *this;
}

GFElement GFElement::operator-() const {
    GFElement result(*this);
    fq_neg(result.element, result.element, result.gf.getCTX());
    return result;
}

GFElement &GFElement::operator++() {
    fq_t one;
    fq_init(one, this->gf.getCTX());
    fq_one(one, this->gf.getCTX());
    fq_add(this->element, this->element, one, this->gf.getCTX());
    fq_clear(one, this->gf.getCTX());
    return *this;
}

GFElement GFElement::operator++(int) {
    GFElement result(*this);
    ++*this;
    return result;
};

GFElement &GFElement::operator--() {
    fq_sub_one(this->element, this->element, this->gf.getCTX());
    return *this;
}

GFElement GFElement::operator--(int) {
    GFElement result(*this);
    --*this;
    return result;
};

GFElement GFElement::operator-(const GFElement &other) const {
    GFElement result(*this);
    result -= other;
    return result;
}

GFElement GFElement::operator+(const GFElement &other) const {
    GFElement result(*this);
    result += other;
    return result;
}

GFElement GFElement::operator*(const GFElement &other) const {
    GFElement result(*this);
    result *= other;
    return result;
}

GFElement GFElement::operator/(const GFElement &other) const {
    GFElement result(*this);
    result /= other;
    return result;
}

GFElement GFElement::operator^(int32_t e) const {
    GFElement result(*this);
    result ^= e;
    return result;
}

std::string GFElement::toString() const {
    char *str = fq_get_str(this->element, this->gf.getCTX());
    std::string result(str);
    flint_free(str);
    return result;
}

bool operator==(const GFElement &a, const GFElement &b) {
    return fq_equal(a.element, b.element, a.gf.getCTX());
}

std::ostream& operator<<(std::ostream &os, const GFElement &elem) {
    os << elem.toString();
    return os;
}

GFElement& GFElement::operator=(const GFElement &other) {
    if (this != &other) fq_set(this->element, other.element, this->gf.getCTX());
    return *this;
}

GFElement::operator uint16_t() const {
    uint16_t result = 0;
    for (slong i = 0; i < fmpz_poly_length(this->element); ++i)
        result = result * *this->gf.Prime() +
            fmpz_poly_get_coeff_ui(this->element, fmpz_poly_degree(this->element) - i);
    return result;
}

GFMatrix::GFMatrix(const GF &gf, slong rows, slong cols) : gf(gf) {
    fq_mat_init(this->mat, rows, cols, this->gf.getCTX());
}

GFMatrix::GFMatrix(const GFMatrix &other) : gf(other.gf) {
    fq_mat_init_set(this->mat, other.mat, this->gf.getCTX());
}

GFMatrix::GFMatrix(const GF &gf, const GFMatrix &other) : gf(gf) {
    fq_mat_init_set(this->mat, other.mat, this->gf.getCTX());
};

GFMatrix::~GFMatrix() {
    fq_mat_clear(mat, this->gf.getCTX());
}

void GFMatrix::one() {
    fq_mat_one(this->mat, this->gf.getCTX());
}

GFElement GFMatrix::operator()(const slong row, const slong col) const {
    return GFElement(this->gf, fq_mat_entry(mat, row, col));
}

void GFMatrix::operator()(const slong row, const slong col, const GFElement &element) {
    fq_mat_entry_set(this->mat, row, col, element.raw(), this->gf.getCTX());
}

void GFMatrix::operator()(const slong row, const slong col, std::uint16_t element) {
    (*this)(row, col, this->gf(element));
}

GFMatrix &GFMatrix::operator+=(const GFMatrix &other) {
    fq_mat_add(this->mat, this->mat, other.mat, this->gf.getCTX());
    return *this;
}

GFMatrix &GFMatrix::operator-=(const GFMatrix &other) {
    fq_mat_sub(this->mat, this->mat, other.mat, this->gf.getCTX());
    return *this;
}

GFMatrix &GFMatrix::operator*=(const GFMatrix &other) {
    fq_mat_t result;
    fq_mat_init(result, this->rows(), other.cols(), this->gf.getCTX());
    fq_mat_mul(result, this->mat, other.mat, this->gf.getCTX());
    fq_mat_swap(this->mat, result, this->gf.getCTX());
    fq_mat_clear(result, this->gf.getCTX());
    return *this;
}

bool GFMatrix::inv() {
    return fq_mat_inv(this->mat, this->mat, this->gf.getCTX());
}

GFMatrix &GFMatrix::operator^=(int32_t e) {
    if (e == 0) {
        this->one();
        return *this;
    }
    if (e < 0) {
        if (!this->inv())
            throw std::invalid_argument("Попытка возвести в отрицательную степень сингулряную матрицу.");
        e = -e;
    }
    GFMatrix base(*this);
    this->one();
    while (e) {
        if (e % 2 == 0) {
            e /= 2;
            base *= base;
        } else {
            --e;
            *this *= base;
        }
    }
    return *this;
}

GFMatrix GFMatrix::operator+(const GFMatrix &other) const {
    GFMatrix result(*this);
    result += other;
    return result;
}

GFMatrix GFMatrix::operator-(const GFMatrix &other) const {
    GFMatrix result(*this);
    result -= other;
    return result;
}

GFMatrix GFMatrix::operator*(const GFMatrix &other) const {
    GFMatrix result(this->gf, this->rows(), other.cols());
    fq_mat_mul(result.mat, this->mat, other.mat, this->gf.getCTX());
    return result;
}

GFMatrix GFMatrix::operator^(int32_t e) const {
    GFMatrix result(*this);
    result ^= e;
    return result;
}

std::string GFMatrix::toString() const {
    std::string result = "[\n";
    for (slong i = 0; i < this->rows(); ++i) {
        result += "\t[ ";
        for (slong j = 0; j < this->cols(); ++j) {
            char* str = fq_get_str_pretty(fq_mat_entry(this->mat, i, j), this->gf.getCTX());
            result += str;
            if (j != this->cols() - 1) result += ", ";
            flint_free(str);
        }
        result += " ]\n";
    }
    result += "]";
    return result;
}

slong GFMatrix::rows() const {
    return fq_mat_nrows(this->mat, this->gf.getCTX());
};
        
slong GFMatrix::cols() const {
    return fq_mat_ncols(this->mat, this->gf.getCTX());
};

bool operator==(const GFMatrix &a, const GFMatrix &b) {
    return fq_mat_equal(a.mat, b.mat, a.gf.getCTX());
}

std::ostream& operator<<(std::ostream &os, const GFMatrix &mat) {
    os << mat.toString();
    return os;
}

GFMatrix &GFMatrix::operator=(const GFMatrix &other) {
    if (this != &other) fq_mat_set(this->mat, other.mat, this->gf.getCTX());
    return *this;
}