#ifndef GF_HPP
#define GF_HPP

#include <flint/fq.h>
#include <flint/fmpz_poly.h>
#include <flint/fq_mat.h>
#include <cstdint>
#include <string>

class GFElement;

class GF {
private:
    fq_ctx_t ctx;
    const std::uint32_t q;

    void getPoly(fmpz_poly_t f, std::uint16_t n) const;
public:
    GF(uint32_t q);
    GF(const GF &other);
    ~GF();
    const fmpz *Prime() const;
    slong Degree() const;
    GFElement operator()(std::uint16_t n) const;
    const fq_ctx_t &getCTX() const;
};

class GFElement {
    private:
        fq_t element;
        const GF &gf;
        
        void inv();
    public:
        GFElement(const GF &gf, const fmpz_poly_t f);
        GFElement(const GFElement &other);
        ~GFElement();
        const fq_t &raw() const;
        GFElement &operator+=(const GFElement &other);
        GFElement &operator-=(const GFElement &other);
        GFElement &operator*=(const GFElement &other);
        GFElement &operator/=(const GFElement &other);
        GFElement &operator^=(int32_t e);
        GFElement operator-() const;
        GFElement &operator++();
        GFElement operator++(int);
        GFElement &operator--();
        GFElement operator--(int);
        GFElement operator-(const GFElement &other) const;
        GFElement operator+(const GFElement &other) const;
        GFElement operator*(const GFElement &other) const;
        GFElement operator/(const GFElement &other) const;
        GFElement operator^(int32_t e) const; // возведение в степень
        std::string toString() const;
        friend bool operator==(const GFElement &a, const GFElement &b);
        GFElement& operator=(const GFElement &other);
};

class GFMatrix {
    private:
        fq_mat_t mat;
        const GF &gf;
    public:
        GFMatrix(const GF &gf, slong rows, slong cols);
        GFMatrix(const GFMatrix &other);
        ~GFMatrix();
        void one();
        GFElement operator()(const slong row, const slong col) const;
        void operator()(const slong row, const slong col, const GFElement &element);
        void operator()(const slong row, const slong col, std::uint16_t element);
        GFMatrix &operator+=(const GFMatrix &other);
        GFMatrix &operator-=(const GFMatrix &other);
        bool inv();
        GFMatrix &operator*=(const GFMatrix &other);
        GFMatrix &operator^=(int32_t e);
        GFMatrix operator-(const GFMatrix &other) const;
        GFMatrix operator+(const GFMatrix &other) const;
        GFMatrix operator*(const GFMatrix &other) const;
        GFMatrix operator^(int32_t e) const; // возведение в степень
        std::string toString() const;
        friend bool operator==(const GFMatrix &a, const GFMatrix &b);
        slong rows() const;
        slong cols() const;
        GFMatrix &operator=(const GFMatrix &other);
    };

std::ostream& operator<<(std::ostream &os, const GFElement &elem);
std::ostream& operator<<(std::ostream &os, const GFMatrix &mat);

#endif