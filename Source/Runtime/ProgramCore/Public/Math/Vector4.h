/*!
 * \file Vector4.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypedefs.h"
#include "ProgramCoreExports.h"
#include "Types/CoreDefines.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>

GLM_HEADER_INCLUDES_BEGIN

class Vector3;

class PROGRAMCORE_EXPORT Vector4
{
private:
    glm::vec4 value{ 0 };
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and matrices
    friend class Matrix4;
    friend Vector4 operator/ (float n, const Vector4 &d);
    friend Vector4 operator- (float n, const Vector4 &d);

public:
    using value_type = glm::vec4::value_type;

public:
    Vector4() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(Vector4)

    Vector4(float x, float y, float z, float w);
    explicit Vector4(const Matrix4Col &vec4);
    explicit Vector4(float allValue);
    Vector4(const Vector3 &xyz, float w);

    float &x();
    float &y();
    float &z();
    float &w();
    float x() const;
    float y() const;
    float z() const;
    float w() const;
    float operator[] (uint32 index) const;
    float &operator[] (uint32 index);

public:
    bool operator== (const Vector4 &b) const;
    float operator| (const Vector4 &b) const;
    // Component wise operations
    Vector4 operator* (const Vector4 &b) const;
    Vector4 &operator*= (const Vector4 &b);
    Vector4 operator/ (const Vector4 &b) const;
    Vector4 &operator/= (const Vector4 &b);
    Vector4 operator- (const Vector4 &b) const;
    Vector4 &operator-= (const Vector4 &b);
    Vector4 operator+ (const Vector4 &b) const;
    Vector4 &operator+= (const Vector4 &b);
    Vector4 operator* (float scalar) const;
    Vector4 &operator*= (float scalar);
    Vector4 operator/ (float scalar) const;
    Vector4 &operator/= (float scalar);
    Vector4 operator- (float scalar) const;
    Vector4 &operator-= (float scalar);
    Vector4 operator+ (float scalar) const;
    Vector4 &operator+= (float scalar);
    Vector4 operator- () const;
    bool isSame(const Vector4 &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    Vector4 safeInverse() const;
    Vector4 normalized() const;
    Vector4 safeNormalized(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;
    float length3() const;
    float sqrlength3() const;

    Vector4 projectTo(const Vector4 &b) const;
    Vector4 rejectFrom(const Vector4 &b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector4 ZERO;
    const static Vector4 ONE;

    static float dot(const Vector4 &a, const Vector4 &b);

    static Vector4 clamp(const Vector4 &value, const Vector4 &min, const Vector4 &max);
    static Vector4 min(const Vector4 &a, const Vector4 &b);
    static Vector4 max(const Vector4 &a, const Vector4 &b);
    static Vector4 abs(const Vector4 &value);
    static Vector4 floor(const Vector4 &value);
    static Vector4 ceil(const Vector4 &value);
    static Vector4 round(const Vector4 &value);
    static Vector4 mod(const Vector4 &a, const Vector4 &b);
    static Vector4 mod(const Vector4 &a, float b);
    static Vector4 modf(Vector4 &wholePart, const Vector4 &value);
};

inline Vector4 operator/ (float n, const Vector4 &d) { return Vector4(n / d.value); }

inline Vector4 operator- (float n, const Vector4 &d) { return Vector4(n - d.value); }

inline Vector4 operator* (float n, const Vector4 &d) { return d * n; }

inline Vector4 operator+ (float n, const Vector4 &d) { return d + n; }

//////////////////////////////////////////////////////////////////////////
/// Implementations
//////////////////////////////////////////////////////////////////////////

inline Vector4::Vector4(const Matrix4Col &val3)
    : value(val3)
{}
inline Vector4::Vector4(float allValue)
    : value(allValue)
{}
inline Vector4::Vector4(float x, float y, float z, float w)
    : value(x, y, z, w)
{}

inline float &Vector4::x() { return value.x; }
inline float Vector4::x() const { return value.x; }

inline float &Vector4::y() { return value.y; }
inline float Vector4::y() const { return value.y; }

inline float &Vector4::z() { return value.z; }
inline float Vector4::z() const { return value.z; }

inline float &Vector4::w() { return value.w; }
inline float Vector4::w() const { return value.w; }

inline float Vector4::operator[] (uint32 index) const { return value[index]; }
inline float &Vector4::operator[] (uint32 index) { return value[index]; }

inline bool Vector4::operator== (const Vector4 &b) const { return isSame(b); }

inline float Vector4::operator| (const Vector4 &b) const { return glm::dot(value, b.value); }

inline Vector4 Vector4::operator* (const Vector4 &b) const { return Vector4(value * b.value); }
inline Vector4 &Vector4::operator*= (const Vector4 &b)
{
    value *= b.value;
    return *this;
}

inline Vector4 Vector4::operator* (float scalar) const { return Vector4(value * scalar); }
inline Vector4 &Vector4::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

inline Vector4 Vector4::operator/ (const Vector4 &b) const { return Vector4(value / b.value); }
inline Vector4 &Vector4::operator/= (const Vector4 &b)
{
    value /= b.value;
    return *this;
}

inline Vector4 Vector4::operator/ (float scalar) const { return Vector4(value / scalar); }
inline Vector4 &Vector4::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

inline Vector4 Vector4::operator- (const Vector4 &b) const { return Vector4(value - b.value); }
inline Vector4 &Vector4::operator-= (const Vector4 &b)
{
    value -= b.value;
    return *this;
}

inline Vector4 Vector4::operator- (float scalar) const { return Vector4(value - scalar); }
inline Vector4 &Vector4::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

inline Vector4 Vector4::operator- () const { return Vector4(-value); }

inline Vector4 Vector4::operator+ (const Vector4 &b) const { return Vector4(value + b.value); }
inline Vector4 &Vector4::operator+= (const Vector4 &b)
{
    value += b.value;
    return *this;
}

inline Vector4 Vector4::operator+ (float scalar) const { return Vector4(value + scalar); }
inline Vector4 &Vector4::operator+= (float scalar)
{
    value += scalar;
    return *this;
}

inline Vector4 Vector4::normalized() const { return Vector4(glm::normalize(value)); }

inline float Vector4::length() const { return glm::length(value); }
inline float Vector4::sqrlength() const { return value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w; }
inline float Vector4::length3() const { return glm::sqrt(sqrlength3()); }
inline float Vector4::sqrlength3() const { return value.x * value.x + value.y * value.y + value.z * value.z; }

inline Vector4 Vector4::projectTo(const Vector4 &b) const { return Vector4(b * (*this | b) / (b | b)); }
inline Vector4 Vector4::rejectFrom(const Vector4 &b) const { return *this - projectTo(b); }

inline float Vector4::dot(const Vector4 &a, const Vector4 &b) { return a | b; }

inline Vector4 Vector4::clamp(const Vector4 &value, const Vector4 &min, const Vector4 &max)
{
    return Vector4(glm::clamp(value.value, min.value, max.value));
}

inline Vector4 Vector4::min(const Vector4 &a, const Vector4 &b) { return Vector4(glm::min(a.value, b.value)); }
inline Vector4 Vector4::max(const Vector4 &a, const Vector4 &b) { return Vector4(glm::max(a.value, b.value)); }

inline Vector4 Vector4::abs(const Vector4 &value) { return Vector4(glm::abs(value.value)); }

inline Vector4 Vector4::floor(const Vector4 &value) { return Vector4(glm::floor(value.value)); }
inline Vector4 Vector4::ceil(const Vector4 &value) { return Vector4(glm::ceil(value.value)); }
inline Vector4 Vector4::round(const Vector4 &value) { return Vector4(glm::round(value.value)); }

inline Vector4 Vector4::mod(const Vector4 &a, const Vector4 &b) { return Vector4(glm::mod(a.value, b.value)); }
inline Vector4 Vector4::mod(const Vector4 &a, float b) { return Vector4(glm::mod(a.value, b)); }
inline Vector4 Vector4::modf(Vector4 &wholePart, const Vector4 &value) { return Vector4(glm::modf(value.value, wholePart.value)); }