/*!
 * \file Rotation.h
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

GLM_HEADER_INCLUDES_BEGIN

#include <glm/ext/vector_float3.hpp>
#include <glm/common.hpp>

GLM_HEADER_INCLUDES_END

class Vector3;

class PROGRAMCORE_EXPORT Rotation
{
private:
    glm::vec3 value{ 0 };

    Rotation(const glm::vec3 &rotValue);

public:
    Rotation() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(Rotation)

    Rotation(float r, float p, float y);
    explicit Rotation(float allValue);

    float &roll();
    float &pitch();
    float &yaw();
    float roll() const;
    float pitch() const;
    float yaw() const;

    Vector3 fwdVector() const;
    Vector3 rightVector() const;
    Vector3 upVector() const;

public:
    bool operator== (const Rotation &b) const;
    // Component wise operations
    Rotation operator* (const Rotation &b) const;
    Rotation &operator*= (const Rotation &b);
    Rotation operator/ (const Rotation &b) const;
    Rotation &operator/= (const Rotation &b);
    Rotation operator- (const Rotation &b) const;
    Rotation &operator-= (const Rotation &b);
    Rotation operator+ (const Rotation &b) const;
    Rotation &operator+= (const Rotation &b);
    Rotation operator* (float scalar) const;
    Rotation &operator*= (float scalar);
    Rotation operator/ (float scalar) const;
    Rotation &operator/= (float scalar);
    Rotation operator- (float scalar) const;
    Rotation &operator-= (float scalar);
    Rotation operator+ (float scalar) const;
    Rotation &operator+= (float scalar);
    bool isSame(const Rotation &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    bool isNan() const;

    static Rotation clamp(const Rotation &value, const Rotation &min, const Rotation &max);
    static Rotation min(const Rotation &a, const Rotation &b);
    static Rotation max(const Rotation &a, const Rotation &b);
    static Rotation abs(const Rotation &value);
    static Rotation sign(const Rotation &value);
    static Rotation floor(const Rotation &value);
    static Rotation ceil(const Rotation &value);
    static Rotation round(const Rotation &value);
    static Rotation mod(const Rotation &a, const Rotation &b);
    static Rotation mod(const Rotation &a, float b);
    static Rotation modf(Rotation &wholePart, const Rotation &value);
};

//////////////////////////////////////////////////////////////////////////
/// Implementations
//////////////////////////////////////////////////////////////////////////

inline Rotation::Rotation(const glm::vec3 &rotValue)
    : value(rotValue)
{}
inline Rotation::Rotation(float allValue)
    : value(allValue)
{}
inline Rotation::Rotation(float r, float p, float y)
    : value(r, p, y)
{}

inline float &Rotation::roll() { return value.x; }
inline float Rotation::roll() const { return value.x; }

inline float &Rotation::pitch() { return value.y; }
inline float Rotation::pitch() const { return value.y; }

inline float &Rotation::yaw() { return value.z; }
inline float Rotation::yaw() const { return value.z; }

inline bool Rotation::operator== (const Rotation &b) const { return isSame(b); }

inline Rotation Rotation::operator* (const Rotation &b) const { return Rotation(value * b.value); }
inline Rotation &Rotation::operator*= (const Rotation &b)
{
    value *= b.value;
    return *this;
}

inline Rotation Rotation::operator* (float scalar) const { return Rotation(value * scalar); }
inline Rotation &Rotation::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

inline Rotation Rotation::operator/ (const Rotation &b) const { return Rotation(value / b.value); }
inline Rotation &Rotation::operator/= (const Rotation &b)
{
    value /= b.value;
    return *this;
}

inline Rotation Rotation::operator/ (float scalar) const { return Rotation(value / scalar); }
inline Rotation &Rotation::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

inline Rotation Rotation::operator- (const Rotation &b) const { return Rotation(value - b.value); }
inline Rotation &Rotation::operator-= (const Rotation &b)
{
    value -= b.value;
    return *this;
}

inline Rotation Rotation::operator- (float scalar) const { return Rotation(value - scalar); }
inline Rotation &Rotation::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

inline Rotation Rotation::operator+ (const Rotation &b) const { return Rotation(value + b.value); }
inline Rotation &Rotation::operator+= (const Rotation &b)
{
    value += b.value;
    return *this;
}

inline Rotation Rotation::operator+ (float scalar) const { return Rotation(value + scalar); }
inline Rotation &Rotation::operator+= (float scalar)
{
    value += scalar;
    return *this;
}

inline Rotation Rotation::clamp(const Rotation &value, const Rotation &min, const Rotation &max)
{
    return Rotation(glm::clamp(value.value, min.value, max.value));
}

inline Rotation Rotation::min(const Rotation &a, const Rotation &b) { return Rotation(glm::min(a.value, b.value)); }
inline Rotation Rotation::max(const Rotation &a, const Rotation &b) { return Rotation(glm::max(a.value, b.value)); }

inline Rotation Rotation::abs(const Rotation &value) { return Rotation(glm::abs(value.value)); }
inline Rotation Rotation::sign(const Rotation &value) { return Rotation(glm::sign(value.value)); }

inline Rotation Rotation::floor(const Rotation &value) { return Rotation(glm::floor(value.value)); }
inline Rotation Rotation::ceil(const Rotation &value) { return Rotation(glm::ceil(value.value)); }
inline Rotation Rotation::round(const Rotation &value) { return Rotation(glm::round(value.value)); }

inline Rotation Rotation::mod(const Rotation &a, const Rotation &b) { return Rotation(glm::mod(a.value, b.value)); }
inline Rotation Rotation::mod(const Rotation &a, float b) { return Rotation(glm::mod(a.value, b)); }
inline Rotation Rotation::modf(Rotation &wholePart, const Rotation &value) { return Rotation(glm::modf(value.value, wholePart.value)); }