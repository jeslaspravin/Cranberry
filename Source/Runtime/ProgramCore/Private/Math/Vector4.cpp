/*!
 * \file Vector4.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector4.h"
#include "Math/Math.h"
#include "Math/Vector3.h"
#include "Types/Platform/PlatformAssertionErrors.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/geometric.hpp>

GLM_HEADER_INCLUDES_END

Vector4::Vector4(const Matrix4Col &Vector3)
    : value(Vector3)
{}

Vector4::Vector4()
    : value(0)
{}

Vector4::Vector4(float allValue)
    : value(allValue)
{}

Vector4::Vector4(float x, float y, float z, float w)
    : value(x, y, z, w)
{}

Vector4::Vector4(const Vector3 &xyz, float w)
    : value(xyz.x(), xyz.y(), xyz.z(), w)
{}

float &Vector4::x() { return value.x; }

float Vector4::x() const { return value.x; }

float &Vector4::y() { return value.y; }

float Vector4::y() const { return value.y; }

float &Vector4::z() { return value.z; }

float Vector4::z() const { return value.z; }

float &Vector4::w() { return value.w; }

float Vector4::w() const { return value.w; }

float Vector4::operator[] (uint32 index) const
{
    debugAssert(index < 4);
    return value[index];
}

float &Vector4::operator[] (uint32 index)
{
    debugAssert(index < 4);
    return value[index];
}

bool Vector4::operator== (const Vector4 &b) const { return isSame(b); }

float Vector4::operator| (const Vector4 &b) const { return glm::dot(value, b.value); }

Vector4 Vector4::operator* (const Vector4 &b) const { return Vector4(value * b.value); }

Vector4 &Vector4::operator*= (const Vector4 &b)
{
    value *= b.value;
    return *this;
}

Vector4 Vector4::operator* (float scalar) const { return Vector4(value * scalar); }

Vector4 &Vector4::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

Vector4 Vector4::operator/ (const Vector4 &b) const { return Vector4(value / b.value); }

Vector4 &Vector4::operator/= (const Vector4 &b)
{
    value /= b.value;
    return *this;
}

Vector4 Vector4::operator/ (float scalar) const { return Vector4(value / scalar); }

Vector4 &Vector4::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

Vector4 Vector4::operator- (const Vector4 &b) const { return Vector4(value - b.value); }

Vector4 &Vector4::operator-= (const Vector4 &b)
{
    value -= b.value;
    return *this;
}

Vector4 Vector4::operator- (float scalar) const { return Vector4(value - scalar); }

Vector4 &Vector4::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

Vector4 Vector4::operator- () const { return Vector4(-value); }

Vector4 Vector4::operator+ (const Vector4 &b) const { return Vector4(value + b.value); }

Vector4 &Vector4::operator+= (const Vector4 &b)
{
    value += b.value;
    return *this;
}

Vector4 Vector4::operator+ (float scalar) const { return Vector4(value + scalar); }

Vector4 &Vector4::operator+= (float scalar)
{
    value += scalar;
    return *this;
}

bool Vector4::isSame(const Vector4 &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
           && Math::isEqual(value.z, b.value.z, epsilon);
}

bool Vector4::isFinite() const
{
    return Math::isFinite(value.x) && Math::isFinite(value.y) && Math::isFinite(value.z) && Math::isFinite(value.w);
}

Vector4 Vector4::safeInverse() const
{
    return Vector4(
        Math::isEqual(value.x, 0.0f) ? 0.0f : 1.0f / value.x, Math::isEqual(value.y, 0.0f) ? 0.0f : 1.0f / value.y,
        Math::isEqual(value.z, 0.0f) ? 0.0f : 1.0f / value.z, Math::isEqual(value.w, 0.0f) ? 0.0f : 1.0f / value.w
    );
}

Vector4 Vector4::normalized() const { return Vector4(glm::normalize(value)); }

Vector4 Vector4::safeNormalized(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < threshold)
    {
        return Vector4::ZERO;
    }
    return Vector4(value * Math::invSqrt(sqrLen));
}

float Vector4::length() const { return glm::length(value); }

float Vector4::sqrlength() const { return Math::pow2(value.x) + Math::pow2(value.y) + Math::pow2(value.z) + Math::pow2(value.w); }

Vector4 Vector4::projectTo(const Vector4 &b) const { return Vector4(b * (*this | b) / (b | b)); }

Vector4 Vector4::rejectFrom(const Vector4 &b) const { return *this - projectTo(b); }

const Vector4 Vector4::ZERO(0);
const Vector4 Vector4::ONE(1);

float Vector4::dot(const Vector4 &a, const Vector4 &b) { return a | b; }

Vector4 Vector4::clamp(const Vector4 &value, const Vector4 &min, const Vector4 &max)
{
    return Vector4(glm::clamp(value.value, min.value, max.value));
}

Vector4 Vector4::min(const Vector4 &a, const Vector4 &b) { return Vector4(glm::min(a.value, b.value)); }

Vector4 Vector4::max(const Vector4 &a, const Vector4 &b) { return Vector4(glm::max(a.value, b.value)); }

Vector4 Vector4::abs(const Vector4 &value) { return Vector4(glm::abs(value.value)); }

Vector4 Vector4::floor(const Vector4 &value) { return Vector4(glm::floor(value.value)); }

Vector4 Vector4::ceil(const Vector4 &value) { return Vector4(glm::ceil(value.value)); }

Vector4 Vector4::round(const Vector4 &value) { return Vector4(glm::round(value.value)); }

Vector4 Vector4::mod(const Vector4 &a, const Vector4 &b) { return Vector4(glm::mod(a.value, b.value)); }

Vector4 Vector4::mod(const Vector4 &a, float b) { return Vector4(glm::mod(a.value, b)); }

Vector4 Vector4::modf(Vector4 &wholePart, const Vector4 &value) { return Vector4(glm::modf(value.value, wholePart.value)); }