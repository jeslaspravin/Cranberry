#pragma once

#include "CoreMathTypedefs.h"

#include <glm/ext/matrix_float3x3.hpp>

class Vector3D;

class Matrix3
{
private:
    glm::mat3 value;
    Matrix3(const glm::mat3& matrix);
public:
    Matrix3();
    explicit Matrix3(float allValue);
    Matrix3(const Vector3D& c1, const Vector3D& c2, const Vector3D& c3);
    Matrix3(const float& c1x, const float& c1y, const float& c1z
        , const float& c2x, const float& c2y, const float& c2z
        , const float& c3x, const float& c3y, const float& c3z);
    Matrix3(const Matrix3& other);
    Matrix3(Matrix3&& other);
    Matrix3(const Vector3D& scale);
    void operator=(const Matrix3& other);
    void operator=(Matrix3&& other);

    Matrix3Col& operator[](uint32 colIndex);
    Matrix3Col operator[](uint32 colIndex) const;

public:
    Vector3D operator*(const Vector3D& transformingVector) const;
    Matrix3 operator*(const Matrix3& b) const;
    void operator*=(const Matrix3& b);

    // Component wise operations
    Matrix3 operator|(const Matrix3& b) const;
    void operator|=(const Matrix3& b);
    Matrix3 operator/(const Matrix3& b) const;
    void operator/=(const Matrix3& b);
    Matrix3 operator-(const Matrix3& b) const;
    void operator-=(const Matrix3& b);
    Matrix3 operator+(const Matrix3& b) const;
    void operator+=(const Matrix3& b);
    Matrix3 operator*(const float& scalar) const;
    void operator*=(const float& scalar);
    Matrix3 operator/(const float& scalar) const;
    void operator/=(const float& scalar);
    Matrix3 operator-(const float& scalar) const;
    void operator-=(const float& scalar);
    Matrix3 operator+(const float& scalar) const;
    void operator+=(const float& scalar);
public:
    static const Matrix3 IDENTITY;
};