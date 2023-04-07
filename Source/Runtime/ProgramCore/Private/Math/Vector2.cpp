/*!
 * \file Vector2.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector2.h"
#include "Math/Math.h"
#include "Math/Vector3.h"
#include "Types/Platform/PlatformAssertionErrors.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>

GLM_HEADER_INCLUDES_END

Vector2::Vector2(const Matrix2Col &Vector2)
    : value(Vector2)
{}

Vector2::Vector2()
    : value(0)
{}

Vector2::Vector2(float x, float y)
    : value(x, y)
{}

Vector2::Vector2(float allValue)
    : value(allValue)
{}

Vector2::Vector2(const Vector3 &other)
    : value(other.x(), other.y())
{}

float &Vector2::x() { return value.x; }

float Vector2::x() const { return value.x; }

float &Vector2::y() { return value.y; }

float Vector2::y() const { return value.y; }

float Vector2::operator[] (uint32 index) const
{
    debugAssert(index < 2);
    return value[index];
}

float &Vector2::operator[] (uint32 index)
{
    debugAssert(index < 2);
    return value[index];
}

bool Vector2::operator== (const Vector2 &b) const { return isSame(b); }

float Vector2::operator| (const Vector2 &b) const { return glm::dot(value, b.value); }

float Vector2::operator^ (const Vector2 &b) const { return glm::cross(glm::vec3(value, 0), glm::vec3(b.value, 0)).z; }

Vector2 Vector2::operator* (const Vector2 &b) const { return Vector2(value * b.value); }

Vector2 &Vector2::operator*= (const Vector2 &b)
{
    value *= b.value;
    return *this;
}

Vector2 Vector2::operator* (float scalar) const { return Vector2(value * scalar); }

Vector2 &Vector2::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

Vector2 Vector2::operator/ (const Vector2 &b) const { return Vector2(value / b.value); }

Vector2 &Vector2::operator/= (const Vector2 &b)
{
    value /= b.value;
    return *this;
}

Vector2 Vector2::operator/ (float scalar) const { return Vector2(value / scalar); }

Vector2 &Vector2::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

Vector2 Vector2::operator- (const Vector2 &b) const { return Vector2(value - b.value); }

Vector2 &Vector2::operator-= (const Vector2 &b)
{
    value -= b.value;
    return *this;
}

Vector2 Vector2::operator- (float scalar) const { return Vector2(value - scalar); }

Vector2 &Vector2::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

Vector2 Vector2::operator- () const { return Vector2(-value); }

Vector2 Vector2::operator+ (const Vector2 &b) const { return Vector2(value + b.value); }

Vector2 &Vector2::operator+= (const Vector2 &b)
{
    value += b.value;
    return *this;
}

Vector2 Vector2::operator+ (float scalar) const { return Vector2(value + scalar); }

Vector2 &Vector2::operator+= (float scalar)
{
    value += scalar;
    return *this;
}

bool Vector2::isSame(const Vector2 &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon);
}

bool Vector2::isFinite() const { return Math::isFinite(value.x) && Math::isFinite(value.y); }

Vector2 Vector2::safeInverse() const
{
    return Vector2(Math::isEqual(value.x, 0.0f) ? 0.0f : 1.0f / value.x, Math::isEqual(value.y, 0.0f) ? 0.0f : 1.0f / value.y);
}

Vector2 Vector2::normalized() const { return Vector2(glm::normalize(value)); }

Vector2 Vector2::safeNormalized(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < threshold)
    {
        return Vector2::ZERO;
    }
    return Vector2(value * Math::invSqrt(sqrLen));
}

float Vector2::length() const { return glm::length(value); }

float Vector2::sqrlength() const { return Math::pow2(value.x) + Math::pow2(value.y); }

Vector2 Vector2::projectTo(const Vector2 &b) const { return Vector2(b * (*this | b) / (b | b)); }

Vector2 Vector2::rejectFrom(const Vector2 &b) const { return *this - projectTo(b); }

const Vector2 Vector2::RIGHT(0, 1);

const Vector2 Vector2::FWD(1, 0);

const Vector2 Vector2::ZERO(0);

const Vector2 Vector2::ONE(1);

float Vector2::dot(const Vector2 &a, const Vector2 &b) { return a | b; }

float Vector2::cross(const Vector2 &a, const Vector2 &b) { return a ^ b; }

Vector2 Vector2::clamp(const Vector2 &value, const Vector2 &min, const Vector2 &max)
{
    return Vector2(glm::clamp(value.value, min.value, max.value));
}

Vector2 Vector2::min(const Vector2 &a, const Vector2 &b) { return Vector2(glm::min(a.value, b.value)); }

Vector2 Vector2::max(const Vector2 &a, const Vector2 &b) { return Vector2(glm::max(a.value, b.value)); }

Vector2 Vector2::abs(const Vector2 &value) { return Vector2(glm::abs(value.value)); }

Vector2 Vector2::floor(const Vector2 &value) { return Vector2(glm::floor(value.value)); }

Vector2 Vector2::ceil(const Vector2 &value) { return Vector2(glm::ceil(value.value)); }

Vector2 Vector2::round(const Vector2 &value) { return Vector2(glm::round(value.value)); }

Vector2 Vector2::mod(const Vector2 &a, const Vector2 &b) { return Vector2(glm::mod(a.value, b.value)); }

Vector2 Vector2::mod(const Vector2 &a, float b) { return Vector2(glm::mod(a.value, b)); }

Vector2 Vector2::modf(Vector2 &wholePart, const Vector2 &value) { return Vector2(glm::modf(value.value, wholePart.value)); }