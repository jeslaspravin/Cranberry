#pragma once

#include "CoreMathTypedefs.h"

#include <glm/ext/vector_float4.hpp>

class Vector4D
{
private:
    glm::vec4 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and matrices
    friend class Matrix4;
public:
    Vector4D();
    Vector4D(const float& x, const float& y, const float& z, const float& w);
    explicit Vector4D(const Matrix4Col& vector4d);
    explicit Vector4D(const float& allValue);
    Vector4D(const Vector4D& other);
    Vector4D(Vector4D&& other);
    void operator=(const Vector4D& other);
    void operator=(Vector4D&& other);

    float& x();
    float& y();
    float& z();
    float& w();
    float x() const;
    float y() const;
    float z() const;
    float w() const;

public:
    bool operator==(const Vector4D& b) const;
    float operator|(const Vector4D& b) const;
    // Component wise operations
    Vector4D operator*(const Vector4D& b) const;
    void operator*=(const Vector4D& b);
    Vector4D operator/(const Vector4D& b) const;
    void operator/=(const Vector4D& b);
    Vector4D operator-(const Vector4D& b) const;
    void operator-=(const Vector4D& b);
    Vector4D operator+(const Vector4D& b) const;
    void operator+=(const Vector4D& b);
    Vector4D operator*(const float& scalar) const;
    void operator*=(const float& scalar);
    Vector4D operator/(const float& scalar) const;
    void operator/=(const float& scalar);
    Vector4D operator-(const float& scalar) const;
    void operator-=(const float& scalar);
    Vector4D operator+(const float& scalar) const;
    void operator+=(const float& scalar);
    bool isSame(const Vector4D& b, float epsilon = SMALL_EPSILON) const;
    Vector4D normalized() const;
    Vector4D safeNormalize(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector4D projectTo(const Vector4D& b) const;
    Vector4D rejectFrom(const Vector4D& b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector4D ZERO;
    const static Vector4D ONE;

    static float dot(const Vector4D& a, const Vector4D& b);
};