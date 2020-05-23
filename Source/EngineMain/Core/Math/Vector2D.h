#pragma once

#include "CoreMathTypedefs.h"

#include <glm/ext/vector_float2.hpp>

class Vector2D
{
private:
    glm::vec2 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and matrices
    friend class Matrix2;
public:
    Vector2D();
    explicit Vector2D(const Matrix2Col& vector2d);
    Vector2D(const float& x, const float& y);
    explicit Vector2D(const float& allValue);
    Vector2D(const Vector2D& other);
    Vector2D(Vector2D&& other);
    void operator=(const Vector2D& other);
    void operator=(Vector2D&& other);

    float& x();
    float& y();
    float x() const;
    float y() const;

public:
    bool operator==(const Vector2D& b) const;
    float operator|(const Vector2D& b) const;
    float operator^(const Vector2D& b) const;
    // Component wise operations
    Vector2D operator*(const Vector2D& b) const;
    void operator*=(const Vector2D& b);
    Vector2D operator/(const Vector2D& b) const;
    void operator/=(const Vector2D& b);
    Vector2D operator-(const Vector2D& b) const;
    void operator-=(const Vector2D& b);
    Vector2D operator+(const Vector2D& b) const;
    void operator+=(const Vector2D& b);
    Vector2D operator*(const float& scalar) const;
    void operator*=(const float& scalar);
    Vector2D operator/(const float& scalar) const;
    void operator/=(const float& scalar);
    Vector2D operator-(const float& scalar) const;
    void operator-=(const float& scalar);
    Vector2D operator+(const float& scalar) const;
    void operator+=(const float& scalar);


    bool isSame(const Vector2D& b, float epsilon = SMALL_EPSILON) const;
    Vector2D normalized() const;
    Vector2D safeNormalize(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector2D projectTo(const Vector2D& b) const;
    Vector2D rejectFrom(const Vector2D& b) const;

//////////////////////////////////////////////////////////////////////////
//// Static functions
//////////////////////////////////////////////////////////////////////////
public:
    const static Vector2D RIGHT;
    const static Vector2D FWD;
    const static Vector2D ZERO;
    const static Vector2D ONE;

    static float dot(const Vector2D& a, const Vector2D& b);
    static float cross(const Vector2D& a, const Vector2D& b);
};
