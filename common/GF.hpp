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
    uint32_t Order() const;
    GFElement operator()(std::uint16_t n) const;
    const fq_ctx_t &getCTX() const;
    std::string toString() const;
};

class GFElement {
    private:
        const GF &gf;
        fq_t element;
        
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
        explicit operator uint16_t() const;
};

class GFMatrix {
private:
    GFMatrix getPinvAlgBlock() const;
protected:
    const GF &gf;
    fq_mat_t mat;
    mutable slong min_poly_degree = - 2;

    // Флаг нужнен, чтобы деструктор GFSubmatrix не уничтожил оригинальную матрицу.
    bool owns = true;
    // Данный конструтор не инициализирует mat. Нужен для создания GFSubmatrix.
    GFMatrix(const GF &gf);
public:
    GFMatrix(const GF &gf, fq_mat_t&& mat);
    GFMatrix(const GF &gf, slong rows, slong cols);
    // Внимание! Берёт ту же ссылку на GF, что хранится в оригинале.
    GFMatrix(const GFMatrix &other);
    // Берёт новую ссылку на GF.
    GFMatrix(const GF &gf, const GFMatrix &other);
    static GFMatrix FromIndex(const GF &gf, slong cols, uint64_t index);
    const fq_mat_t &raw() const;
    ~GFMatrix();
    GFMatrix &one();
    // Внимание! gf в вернувшемся элементе является ссылкой на поле из этого автомата.
    GFElement operator()(const slong row, const slong col) const;
    GFMatrix &operator()(const slong row, const slong col, const GFElement &element);
    GFMatrix &operator()(const slong row, const slong col, std::uint16_t element);
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
    explicit operator uint64_t() const;
    slong minPolyDegree() const;
    slong rank() const;
    bool isZero() const;
    slong reduce(GFMatrix &reduced) const;
    const GF &getGF() const;
    GFMatrix pseudoInv() const;
    GFMatrix &swapRows(slong i, slong j);
    GFMatrix &swapCols(slong i, slong j);
    GFMatrix transpose() const;
};

std::ostream& operator<<(std::ostream &os, const GF &gf);
std::ostream& operator<<(std::ostream &os, const GFElement &elem);
std::ostream& operator<<(std::ostream &os, const GFMatrix &mat);

class GFSubmatrix : public GFMatrix {
public:
    GFSubmatrix(const GFMatrix &mat, slong r1, slong c1, slong r2, slong c2);
    ~GFSubmatrix();
    GFSubmatrix &operator=(const GFMatrix &other);
};

#endif