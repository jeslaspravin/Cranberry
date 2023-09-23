/*!
 * \file Vector2.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypedefs.h"
#include "ProgramCoreExports.h"
#include "Types/CoreDefines.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>

GLM_HEADER_INCLUDES_BEGIN

class Vector3;

class PROGRAMCORE_EXPORT Vector2
{
private:
    glm::vec2 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and matrices
    friend class Matrix2;
    friend Vector2 operator/ (float n, const Vector2 &d);
    friend Vector2 operator- (float n, const Vector2 &d);

public:
    using value_type = glm::vec2::value_type;

public:
    Vector2() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(Vector2)
    explicit Vector2(const Matrix2Col &Vector2);
    Vector2(float x, float y);
    explicit Vector2(float allValue);
    Vector2(const Vector3 &other);

    float &x();
    float &y();
    float x() const;
    float y() const;
    float operator[] (uint32 index) const;
    float &operator[] (uint32 index);

public:
    bool operator== (const Vector2 &b) const;
    float operator| (const Vector2 &b) const;
    float operator^ (const Vector2 &b) const;
    // Component wise operations
    Vector2 operator* (const Vector2 &b) const;
    Vector2 &operator*= (const Vector2 &b);
    Vector2 operator/ (const Vector2 &b) const;
    Vector2 &operator/= (const Vector2 &b);
    Vector2 operator- (const Vector2 &b) const;
    Vector2 &operator-= (const Vector2 &b);
    Vector2 operator+ (const Vector2 &b) const;
    Vector2 &operator+= (const Vector2 &b);
    Vector2 operator* (float scalar) const;
    Vector2 &operator*= (float scalar);
    Vector2 operator/ (float scalar) const;
    Vector2 &operator/= (float scalar);
    Vector2 operator- (float scalar) const;
    Vector2 &operator-= (float scalar);
    Vector2 operator+ (float scalar) const;
    Vector2 &operator+= (float scalar);
    Vector2 operator- () const;

    bool isSame(const Vector2 &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    bool isNan() const;
    Vector2 safeInverse() const;
    Vector2 normalized() const;
    Vector2 safeNormalized(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector2 projectTo(const Vector2 &b) const;
    Vector2 rejectFrom(const Vector2 &b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector2 RIGHT;
    const static Vector2 FWD;
    const static Vector2 ZERO;
    const static Vector2 ONE;

    static float dot(const Vector2 &a, const Vector2 &b);
    static float cross(const Vector2 &a, const Vector2 &b);

    static Vector2 clamp(const Vector2 &value, const Vector2 &min, const Vector2 &max);
    static Vector2 min(const Vector2 &a, const Vector2 &b);
    static Vector2 max(const Vector2 &a, const Vector2 &b);
    static Vector2 abs(const Vector2 &value);
    static Vector2 sign(const Vector2 &value);
    static Vector2 floor(const Vector2 &value);
    static Vector2 ceil(const Vector2 &value);
    static Vector2 round(const Vector2 &value);
    static Vector2 mod(const Vector2 &a, const Vector2 &b);
    static Vector2 mod(const Vector2 &a, float b);
    static Vector2 modf(Vector2 &wholePart, const Vector2 &value);
};

FORCE_INLINE Vector2 operator/ (float n, const Vector2 &d) { return Vector2(n / d.value); }

FORCE_INLINE Vector2 operator- (float n, const Vector2 &d) { return Vector2(n - d.value); }

FORCE_INLINE Vector2 operator* (float n, const Vector2 &d) { return d * n; }

FORCE_INLINE Vector2 operator+ (float n, const Vector2 &d) { return d + n; }

//////////////////////////////////////////////////////////////////////////
/// Implementations
//////////////////////////////////////////////////////////////////////////

inline Vector2::Vector2(const Matrix2Col &Vector2)
    : value(Vector2)
{}
inline Vector2::Vector2(float x, float y)
    : value(x, y)
{}
inline Vector2::Vector2(float allValue)
    : value(allValue)
{}

inline float &Vector2::x() { return value.x; }
inline float Vector2::x() const { return value.x; }

inline float &Vector2::y() { return value.y; }
inline float Vector2::y() const { return value.y; }

inline float Vector2::operator[] (uint32 index) const { return value[index]; }
inline float &Vector2::operator[] (uint32 index) { return value[index]; }

inline bool Vector2::operator== (const Vector2 &b) const { return isSame(b); }

inline float Vector2::operator| (const Vector2 &b) const { return glm::dot(value, b.value); }
inline float Vector2::operator^ (const Vector2 &b) const { return value.x * b.value.y - value.y * b.value.x; }

inline Vector2 Vector2::operator* (const Vector2 &b) const { return Vector2(value * b.value); }
inline Vector2 &Vector2::operator*= (const Vector2 &b)
{
    value *= b.value;
    return *this;
}

inline Vector2 Vector2::operator* (float scalar) const { return Vector2(value * scalar); }
inline Vector2 &Vector2::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

inline Vector2 Vector2::operator/ (const Vector2 &b) const { return Vector2(value / b.value); }
inline Vector2 &Vector2::operator/= (const Vector2 &b)
{
    value /= b.value;
    return *this;
}

inline Vector2 Vector2::operator/ (float scalar) const { return Vector2(value / scalar); }
inline Vector2 &Vector2::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

inline Vector2 Vector2::operator- (const Vector2 &b) const { return Vector2(value - b.value); }
inline Vector2 &Vector2::operator-= (const Vector2 &b)
{
    value -= b.value;
    return *this;
}

inline Vector2 Vector2::operator- (float scalar) const { return Vector2(value - scalar); }
inline Vector2 &Vector2::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

inline Vector2 Vector2::operator- () const { return Vector2(-value); }

inline Vector2 Vector2::operator+ (const Vector2 &b) const { return Vector2(value + b.value); }
inline Vector2 &Vector2::operator+= (const Vector2 &b)
{
    value += b.value;
    return *this;
}

inline Vector2 Vector2::operator+ (float scalar) const { return Vector2(value + scalar); }
inline Vector2 &Vector2::operator+= (float scalar)
{
    value += scalar;
    return *this;
}

inline Vector2 Vector2::normalized() const { return Vector2(glm::normalize(value)); }

inline float Vector2::length() const { return glm::length(value); }
inline float Vector2::sqrlength() const { return value.x * value.x + value.y * value.y; }

inline Vector2 Vector2::projectTo(const Vector2 &b) const { return Vector2(b * (*this | b) / (b | b)); }
inline Vector2 Vector2::rejectFrom(const Vector2 &b) const { return *this - projectTo(b); }

inline float Vector2::dot(const Vector2 &a, const Vector2 &b) { return a | b; }
inline float Vector2::cross(const Vector2 &a, const Vector2 &b) { return a ^ b; }

inline Vector2 Vector2::clamp(const Vector2 &value, const Vector2 &min, const Vector2 &max)
{
    return Vector2(glm::clamp(value.value, min.value, max.value));
}

inline Vector2 Vector2::min(const Vector2 &a, const Vector2 &b) { return Vector2(glm::min(a.value, b.value)); }
inline Vector2 Vector2::max(const Vector2 &a, const Vector2 &b) { return Vector2(glm::max(a.value, b.value)); }

inline Vector2 Vector2::abs(const Vector2 &value) { return Vector2(glm::abs(value.value)); }
inline Vector2 Vector2::sign(const Vector2 &value) { return Vector2(glm::sign(value.value)); }

inline Vector2 Vector2::floor(const Vector2 &value) { return Vector2(glm::floor(value.value)); }
inline Vector2 Vector2::ceil(const Vector2 &value) { return Vector2(glm::ceil(value.value)); }
inline Vector2 Vector2::round(const Vector2 &value) { return Vector2(glm::round(value.value)); }

inline Vector2 Vector2::mod(const Vector2 &a, const Vector2 &b) { return Vector2(glm::mod(a.value, b.value)); }
inline Vector2 Vector2::mod(const Vector2 &a, float b) { return Vector2(glm::mod(a.value, b)); }
inline Vector2 Vector2::modf(Vector2 &wholePart, const Vector2 &value) { return Vector2(glm::modf(value.value, wholePart.value)); }