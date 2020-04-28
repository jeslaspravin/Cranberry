#pragma once

#include "CoreMathTypedefs.h"

#include <glm/ext/vector_float2.hpp>

class Vector2D
{
private:
    glm::vec2 value;

    Vector2D(const glm::vec2& vector2d);
public:
    Vector2D();
    Vector2D(const float& x, const float& y);
    explicit Vector2D(const float& allValue);
    Vector2D(const Vector2D& other);
    Vector2D(Vector2D&& other);

    float& x();
    float& y();
    float x() const;
    float y() const;

public:
    bool operator==(const Vector2D& b) const;
    float operator|(const Vector2D& b) const;
    float operator^(const Vector2D& b) const;
    bool isSame(const Vector2D& b, float epsilon = SMALL_EPSILON) const;
    Vector2D normalized();
    Vector2D safeNormalize(float threshold = SMALL_EPSILON);
    float length();
    float sqrlength();

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
    static bool isEqual(const Vector2D& a, const Vector2D& b, float epsilon = SMALL_EPSILON);
};
