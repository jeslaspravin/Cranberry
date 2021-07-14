#pragma once

#include "CoreMathTypedefs.h"

#include <glm/ext/matrix_float2x2.hpp>

class Vector2D;

class Matrix2
{
private:
    glm::mat2 value;
    Matrix2(const glm::mat2& matrix);
public:
    Matrix2();
    Matrix2(float allValue);
    Matrix2(float c1x,float c1y, float c2x,float c2y);
    Matrix2(const Vector2D& c1, const Vector2D& c2);
    Matrix2(const Vector2D& scale);
    Matrix2(const Matrix2& other);
    Matrix2(Matrix2&& other);
    void operator=(const Matrix2& other);
    void operator=(Matrix2&& other);

    Matrix2Col& operator[](uint32 colIndex);
    Matrix2Col operator[](uint32 colIndex) const;

public:
    Vector2D operator*(const Vector2D& transformingVector) const;
    Matrix2 operator*(const Matrix2& b) const;
    void operator*=(const Matrix2& b);

    Matrix2 inverse() const;
    float determinant() const;
    Matrix2 transpost() const;

    // Component wise operations
    Matrix2 operator|(const Matrix2& b) const;
    void operator|=(const Matrix2& b);
    Matrix2 operator/(const Matrix2& b) const;
    void operator/=(const Matrix2& b);
    Matrix2 operator-(const Matrix2& b) const;
    void operator-=(const Matrix2& b);
    Matrix2 operator+(const Matrix2& b) const;
    void operator+=(const Matrix2& b);
    Matrix2 operator*(const float& scalar) const;
    void operator*=(const float& scalar);
    Matrix2 operator/(const float& scalar) const;
    void operator/=(const float& scalar);
    Matrix2 operator-(const float& scalar) const;
    void operator-=(const float& scalar);
    Matrix2 operator+(const float& scalar) const;
    void operator+=(const float& scalar);
    Matrix2 operator-() const;
public:
    static const Matrix2 IDENTITY;
};