#pragma once

#include "CoreMathTypedefs.h"

#include <glm/ext/matrix_float4x4.hpp>

class Vector3D;
class Vector4D;

class Matrix4
{
private:
    glm::mat4 value;
    Matrix4(const glm::mat4& matrix);
public:
    Matrix4();
    explicit Matrix4(float allValue);

    //  Matrix arrangement
    //  0-3B    4-7B    8-11B   12-15B
    //  r0(c0)  r0(c1)  r0(c2)  r0(c3)
    //  r1(c0)  r1(c1)  r1(c2)  r1(c3)
    //  r2(c0)  r2(c1)  r2(c2)  r2(c3)
    //  r3(c0)  r3(c1)  r3(c2)  r3(c3)
    Matrix4(const float& c1x, const float& c1y, const float& c1z, const float& c1w
        , const float& c2x, const float& c2y, const float& c2z, const float& c2w
        , const float& c3x, const float& c3y, const float& c3z, const float& c3w
        , const float& c4x, const float& c4y, const float& c4z, const float& c4w);
    Matrix4(const Vector3D& c1, const Vector3D& c2, const Vector3D& c3, const Vector3D& c4, const float& c4w = 1.0f);
    Matrix4(const Vector4D& c1, const Vector4D& c2, const Vector4D& c3, const Vector4D& c4);
    // Scaling matrix should always be applied before rotation or translation to preserve volume
    Matrix4(const Vector3D& scale);
    Matrix4(const Matrix4& other);
    Matrix4(Matrix4&& other);
    void operator=(const Matrix4& other);
    void operator=(Matrix4&& other);

    Matrix4Col& operator[](uint32 colIndex);
    Matrix4Col operator[](uint32 colIndex) const;

public:
    Vector4D operator*(const Vector4D& transformingVector) const;
    Vector3D operator*(const Vector3D& transformingVector) const;
    Matrix4 operator*(const Matrix4& b) const;
    void operator*=(const Matrix4& b);

    Matrix4 inverse() const;
    float determinant() const;
    Matrix4 transpose() const;

    // Component wise operations
    Matrix4 operator|(const Matrix4& b) const;
    void operator|=(const Matrix4& b);
    Matrix4 operator/(const Matrix4& b) const;
    void operator/=(const Matrix4& b);
    Matrix4 operator-(const Matrix4& b) const;
    void operator-=(const Matrix4& b);
    Matrix4 operator+(const Matrix4& b) const;
    void operator+=(const Matrix4& b);
    Matrix4 operator*(const float& scalar) const;
    void operator*=(const float& scalar);
    Matrix4 operator/(const float& scalar) const;
    void operator/=(const float& scalar);
    Matrix4 operator-(const float& scalar) const;
    void operator-=(const float& scalar);
    Matrix4 operator+(const float& scalar) const;
    void operator+=(const float& scalar);

public:
    static const Matrix4 IDENTITY;
};