/*!
 * \file Vector4D.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector4D.h"
#include "Math/Math.h"
#include "Math/Vector3D.h"
#include "Types/Platform/PlatformAssertionErrors.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/geometric.hpp>

GLM_HEADER_INCLUDES_END

Vector4D::Vector4D(const Matrix4Col &vector3d)
    : value(vector3d)
{}

Vector4D::Vector4D()
    : value(0)
{}

Vector4D::Vector4D(float allValue)
    : value(allValue)
{}

Vector4D::Vector4D(const Vector4D &other)
    : value(other.value)
{}

Vector4D::Vector4D(Vector4D &&other)
    : value(std::move(other.value))
{}

Vector4D::Vector4D(float x, float y, float z, float w)
    : value(x, y, z, w)
{}

Vector4D::Vector4D(const Vector3D &xyz, float w)
    : value(xyz.x(), xyz.y(), xyz.z(), w)
{}

Vector4D &Vector4D::operator=(const Vector4D &other)
{
    value = other.value;
    return *this;
}

Vector4D &Vector4D::operator=(Vector4D &&other)
{
    value = std::move(other.value);
    return *this;
}

float &Vector4D::x() { return value.x; }

float Vector4D::x() const { return value.x; }

float &Vector4D::y() { return value.y; }

float Vector4D::y() const { return value.y; }

float &Vector4D::z() { return value.z; }

float Vector4D::z() const { return value.z; }

float &Vector4D::w() { return value.w; }

float Vector4D::w() const { return value.w; }

float Vector4D::operator[](uint32 index) const
{
    debugAssert(index < 4);
    return value[index];
}

float &Vector4D::operator[](uint32 index)
{
    debugAssert(index < 4);
    return value[index];
}

bool Vector4D::operator==(const Vector4D &b) const { return isSame(b); }

float Vector4D::operator|(const Vector4D &b) const { return glm::dot(value, b.value); }

Vector4D Vector4D::operator*(const Vector4D &b) const { return Vector4D(value * b.value); }

Vector4D &Vector4D::operator*=(const Vector4D &b)
{
    value *= b.value;
    return *this;
}

Vector4D Vector4D::operator*(float scalar) const { return Vector4D(value * scalar); }

Vector4D &Vector4D::operator*=(float scalar)
{
    value *= scalar;
    return *this;
}

Vector4D Vector4D::operator/(const Vector4D &b) const { return Vector4D(value / b.value); }

Vector4D &Vector4D::operator/=(const Vector4D &b)
{
    value /= b.value;
    return *this;
}

Vector4D Vector4D::operator/(float scalar) const { return Vector4D(value / scalar); }

Vector4D &Vector4D::operator/=(float scalar)
{
    value /= scalar;
    return *this;
}

Vector4D Vector4D::operator-(const Vector4D &b) const { return Vector4D(value - b.value); }

Vector4D &Vector4D::operator-=(const Vector4D &b)
{
    value -= b.value;
    return *this;
}

Vector4D Vector4D::operator-(float scalar) const { return Vector4D(value - scalar); }

Vector4D &Vector4D::operator-=(float scalar)
{
    value -= scalar;
    return *this;
}

Vector4D Vector4D::operator-() const { return Vector4D(-value); }

Vector4D Vector4D::operator+(const Vector4D &b) const { return Vector4D(value + b.value); }

Vector4D &Vector4D::operator+=(const Vector4D &b)
{
    value += b.value;
    return *this;
}

Vector4D Vector4D::operator+(float scalar) const { return Vector4D(value + scalar); }

Vector4D &Vector4D::operator+=(float scalar)
{
    value += scalar;
    return *this;
}

bool Vector4D::isSame(const Vector4D &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
           && Math::isEqual(value.z, b.value.z, epsilon);
}

bool Vector4D::isFinite() const
{
    return Math::isFinite(value.x) && Math::isFinite(value.y) && Math::isFinite(value.z) && Math::isFinite(value.w);
}

Vector4D Vector4D::safeInverse() const
{
    return Vector4D(
        Math::isEqual(value.x, 0.0f) ? 0.0f : 1.0f / value.x, Math::isEqual(value.y, 0.0f) ? 0.0f : 1.0f / value.y,
        Math::isEqual(value.z, 0.0f) ? 0.0f : 1.0f / value.z, Math::isEqual(value.w, 0.0f) ? 0.0f : 1.0f / value.w
    );
}

Vector4D Vector4D::normalized() const { return Vector4D(glm::normalize(value)); }

Vector4D Vector4D::safeNormalized(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < threshold)
    {
        return Vector4D::ZERO;
    }
    return Vector4D(value * Math::invSqrt(sqrLen));
}

float Vector4D::length() const { return glm::length(value); }

float Vector4D::sqrlength() const { return Math::pow2(value.x) + Math::pow2(value.y) + Math::pow2(value.z) + Math::pow2(value.w); }

Vector4D Vector4D::projectTo(const Vector4D &b) const { return Vector4D(b * (*this | b) / (b | b)); }

Vector4D Vector4D::rejectFrom(const Vector4D &b) const { return *this - projectTo(b); }

const Vector4D Vector4D::ZERO(0);
const Vector4D Vector4D::ONE(1);

float Vector4D::dot(const Vector4D &a, const Vector4D &b) { return a | b; }

Vector4D Vector4D::clamp(const Vector4D &value, const Vector4D &min, const Vector4D &max)
{
    return Vector4D(glm::clamp(value.value, min.value, max.value));
}

Vector4D Vector4D::min(const Vector4D &a, const Vector4D &b) { return Vector4D(glm::min(a.value, b.value)); }

Vector4D Vector4D::max(const Vector4D &a, const Vector4D &b) { return Vector4D(glm::max(a.value, b.value)); }

Vector4D Vector4D::abs(const Vector4D &value) { return Vector4D(glm::abs(value.value)); }

Vector4D Vector4D::floor(const Vector4D &value) { return Vector4D(glm::floor(value.value)); }

Vector4D Vector4D::ceil(const Vector4D &value) { return Vector4D(glm::ceil(value.value)); }

Vector4D Vector4D::round(const Vector4D &value) { return Vector4D(glm::round(value.value)); }

Vector4D Vector4D::mod(const Vector4D &a, const Vector4D &b) { return Vector4D(glm::mod(a.value, b.value)); }

Vector4D Vector4D::mod(const Vector4D &a, float b) { return Vector4D(glm::mod(a.value, b)); }

Vector4D Vector4D::modf(Vector4D &wholePart, const Vector4D &value) { return Vector4D(glm::modf(value.value, wholePart.value)); }