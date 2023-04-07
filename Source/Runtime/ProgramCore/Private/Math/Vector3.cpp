/*!
 * \file Vector3.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector3.h"
#include "Math/Math.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Types/Platform/PlatformAssertionErrors.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/geometric.hpp>

GLM_HEADER_INCLUDES_END

Vector3::Vector3(const Matrix3Col &Vector3)
    : value(Vector3)
{}

Vector3::Vector3()
    : value(0)
{}

Vector3::Vector3(float x, float y)
    : value(x, y, 0)
{}

Vector3::Vector3(float allValue)
    : value(allValue)
{}

Vector3::Vector3(float x, float y, float z)
    : value(x, y, z)
{}

Vector3::Vector3(const Vector4 &other)
    : value(other.x(), other.y(), other.z())
{}

Vector3::Vector3(const Vector2 &xy, float z)
    : value(xy.x(), xy.y(), z)
{}

float &Vector3::x() { return value.x; }

float Vector3::x() const { return value.x; }

float &Vector3::y() { return value.y; }

float Vector3::y() const { return value.y; }

float &Vector3::z() { return value.z; }

float Vector3::z() const { return value.z; }

float Vector3::operator[] (uint32 index) const
{
    debugAssert(index < 3);
    return value[index];
}

float &Vector3::operator[] (uint32 index)
{
    debugAssert(index < 3);
    return value[index];
}

bool Vector3::operator== (const Vector3 &b) const { return isSame(b); }

float Vector3::operator| (const Vector3 &b) const { return glm::dot(value, b.value); }

Vector3 Vector3::operator^ (const Vector3 &b) const { return Vector3(glm::cross(value, b.value)); }

Vector3 Vector3::operator* (const Vector3 &b) const { return Vector3(value * b.value); }

Vector3 &Vector3::operator*= (const Vector3 &b)
{
    value *= b.value;
    return *this;
}

Vector3 Vector3::operator* (float scalar) const { return Vector3(value * scalar); }

Vector3 &Vector3::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

Vector3 Vector3::operator/ (const Vector3 &b) const { return Vector3(value / b.value); }

Vector3 &Vector3::operator/= (const Vector3 &b)
{
    value /= b.value;
    return *this;
}

Vector3 Vector3::operator/ (float scalar) const { return Vector3(value / scalar); }

Vector3 &Vector3::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

Vector3 Vector3::operator- (const Vector3 &b) const { return Vector3(value - b.value); }

Vector3 &Vector3::operator-= (const Vector3 &b)
{
    value -= b.value;
    return *this;
}

Vector3 Vector3::operator- (float scalar) const { return Vector3(value - scalar); }

Vector3 &Vector3::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

Vector3 Vector3::operator- () const { return Vector3(-value); }

Vector3 Vector3::operator+ (const Vector3 &b) const { return Vector3(value + b.value); }

Vector3 &Vector3::operator+= (const Vector3 &b)
{
    value += b.value;
    return *this;
}

Vector3 Vector3::operator+ (float scalar) const { return Vector3(value + scalar); }

Vector3 &Vector3::operator+= (float scalar)
{
    value += scalar;
    return *this;
}

bool Vector3::isSame(const Vector3 &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
           && Math::isEqual(value.z, b.value.z, epsilon);
}

bool Vector3::isFinite() const { return Math::isFinite(value.x) && Math::isFinite(value.y) && Math::isFinite(value.z); }

Vector3 Vector3::safeInverse() const
{
    return Vector3(
        Math::isEqual(value.x, 0.0f) ? 0.0f : 1.0f / value.x, Math::isEqual(value.y, 0.0f) ? 0.0f : 1.0f / value.y,
        Math::isEqual(value.z, 0.0f) ? 0.0f : 1.0f / value.z
    );
}

Vector3 Vector3::normalized() const { return Vector3(glm::normalize(value)); }

Vector3 Vector3::safeNormalized(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < threshold)
    {
        return Vector3::ZERO;
    }
    return Vector3(value * Math::invSqrt(sqrLen));
}

float Vector3::length() const { return glm::length(value); }

float Vector3::sqrlength() const { return Math::pow2(value.x) + Math::pow2(value.y) + Math::pow2(value.z); }

Vector3 Vector3::projectTo(const Vector3 &b) const { return Vector3(b * (*this | b) / (b | b)); }

Vector3 Vector3::rejectFrom(const Vector3 &b) const { return *this - projectTo(b); }

const Vector3 Vector3::RIGHT(0, 1);

const Vector3 Vector3::FWD(1, 0);

const Vector3 Vector3::UP(0, 0, 1);

const Vector3 Vector3::ZERO(0);

const Vector3 Vector3::ONE(1);

float Vector3::dot(const Vector3 &a, const Vector3 &b) { return a | b; }

Vector3 Vector3::cross(const Vector3 &a, const Vector3 &b) { return a ^ b; }

Vector3 Vector3::clamp(const Vector3 &value, const Vector3 &min, const Vector3 &max)
{
    return Vector3(glm::clamp(value.value, min.value, max.value));
}

Vector3 Vector3::min(const Vector3 &a, const Vector3 &b) { return Vector3(glm::min(a.value, b.value)); }

Vector3 Vector3::max(const Vector3 &a, const Vector3 &b) { return Vector3(glm::max(a.value, b.value)); }

Vector3 Vector3::abs(const Vector3 &value) { return Vector3(glm::abs(value.value)); }

Vector3 Vector3::floor(const Vector3 &value) { return Vector3(glm::floor(value.value)); }

Vector3 Vector3::ceil(const Vector3 &value) { return Vector3(glm::ceil(value.value)); }

Vector3 Vector3::round(const Vector3 &value) { return Vector3(glm::round(value.value)); }

Vector3 Vector3::mod(const Vector3 &a, const Vector3 &b) { return Vector3(glm::mod(a.value, b.value)); }

Vector3 Vector3::mod(const Vector3 &a, float b) { return Vector3(glm::mod(a.value, b)); }

Vector3 Vector3::modf(Vector3 &wholePart, const Vector3 &value) { return Vector3(glm::modf(value.value, wholePart.value)); }