/*!
 * \file Vector3D.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector3D.h"
#include "Math/Math.h"
#include "Math/Vector2D.h"
#include "Math/Vector4D.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#include <glm/geometric.hpp>

Vector3D::Vector3D(const Matrix3Col &vector3d)
    : value(vector3d)
{}

Vector3D::Vector3D()
    : value(0)
{}

Vector3D::Vector3D(const float &x, const float &y)
    : value(x, y, 0)
{}

Vector3D::Vector3D(const float &allValue)
    : value(allValue)
{}

Vector3D::Vector3D(const Vector3D &other)
    : value(other.value)
{}

Vector3D::Vector3D(Vector3D &&other)
    : value(std::move(other.value))
{}

Vector3D::Vector3D(const float &x, const float &y, const float &z)
    : value(x, y, z)
{}

Vector3D::Vector3D(const Vector4D &other)
    : value(other.x(), other.y(), other.z())
{}

Vector3D::Vector3D(const Vector2D &xy, const float &z)
    : value(xy.x(), xy.y(), z)
{}

Vector3D &Vector3D::operator=(const Vector3D &other)
{
    value = other.value;
    return *this;
}

Vector3D &Vector3D::operator=(Vector3D &&other)
{
    value = std::move(other.value);
    return *this;
}

float &Vector3D::x() { return value.x; }

float Vector3D::x() const { return value.x; }

float &Vector3D::y() { return value.y; }

float Vector3D::y() const { return value.y; }

float &Vector3D::z() { return value.z; }

float Vector3D::z() const { return value.z; }

float Vector3D::operator[](uint32 index) const
{
    debugAssert(index < 3);
    return value[index];
}

float &Vector3D::operator[](uint32 index)
{
    debugAssert(index < 3);
    return value[index];
}

bool Vector3D::operator==(const Vector3D &b) const { return isSame(b); }

float Vector3D::operator|(const Vector3D &b) const { return glm::dot(value, b.value); }

Vector3D Vector3D::operator^(const Vector3D &b) const { return Vector3D(glm::cross(value, b.value)); }

Vector3D Vector3D::operator*(const Vector3D &b) const { return Vector3D(value * b.value); }

Vector3D &Vector3D::operator*=(const Vector3D &b)
{
    value *= b.value;
    return *this;
}

Vector3D Vector3D::operator*(const float &scalar) const { return Vector3D(value * scalar); }

Vector3D &Vector3D::operator*=(const float &scalar)
{
    value *= scalar;
    return *this;
}

Vector3D Vector3D::operator/(const Vector3D &b) const { return Vector3D(value / b.value); }

Vector3D &Vector3D::operator/=(const Vector3D &b)
{
    value /= b.value;
    return *this;
}

Vector3D Vector3D::operator/(const float &scalar) const { return Vector3D(value / scalar); }

Vector3D &Vector3D::operator/=(const float &scalar)
{
    value /= scalar;
    return *this;
}

Vector3D Vector3D::operator-(const Vector3D &b) const { return Vector3D(value - b.value); }

Vector3D &Vector3D::operator-=(const Vector3D &b)
{
    value -= b.value;
    return *this;
}

Vector3D Vector3D::operator-(const float &scalar) const { return Vector3D(value - scalar); }

Vector3D &Vector3D::operator-=(const float &scalar)
{
    value -= scalar;
    return *this;
}

Vector3D Vector3D::operator-() const { return Vector3D(-value); }

Vector3D Vector3D::operator+(const Vector3D &b) const { return Vector3D(value + b.value); }

Vector3D &Vector3D::operator+=(const Vector3D &b)
{
    value += b.value;
    return *this;
}

Vector3D Vector3D::operator+(const float &scalar) const { return Vector3D(value + scalar); }

Vector3D &Vector3D::operator+=(const float &scalar)
{
    value += scalar;
    return *this;
}

bool Vector3D::isSame(const Vector3D &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
           && Math::isEqual(value.z, b.value.z, epsilon);
}

bool Vector3D::isFinite() const { return Math::isFinite(value.x) && Math::isFinite(value.y) && Math::isFinite(value.z); }

Vector3D Vector3D::normalized() const { return Vector3D(glm::normalize(value)); }

Vector3D Vector3D::safeNormalize(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < SMALL_EPSILON)
    {
        return Vector3D::ZERO;
    }
    return Vector3D(value * Math::invSqrt(sqrLen));
}

float Vector3D::length() const { return glm::length(value); }

float Vector3D::sqrlength() const { return Math::pow2(value.x) + Math::pow2(value.y) + Math::pow2(value.z); }

Vector3D Vector3D::projectTo(const Vector3D &b) const { return Vector3D(b * (*this | b) / (b | b)); }

Vector3D Vector3D::rejectFrom(const Vector3D &b) const { return *this - projectTo(b); }

const Vector3D Vector3D::RIGHT(0, 1);

const Vector3D Vector3D::FWD(1, 0);

const Vector3D Vector3D::UP(0, 0, 1);

const Vector3D Vector3D::ZERO(0);

const Vector3D Vector3D::ONE(1);

float Vector3D::dot(const Vector3D &a, const Vector3D &b) { return a | b; }

Vector3D Vector3D::cross(const Vector3D &a, const Vector3D &b) { return a ^ b; }

Vector3D Vector3D::clamp(const Vector3D &value, const Vector3D &min, const Vector3D &max)
{
    return Vector3D(glm::clamp(value.value, min.value, max.value));
}

Vector3D Vector3D::min(const Vector3D &a, const Vector3D &b) { return Vector3D(glm::min(a.value, b.value)); }

Vector3D Vector3D::max(const Vector3D &a, const Vector3D &b) { return Vector3D(glm::max(a.value, b.value)); }

Vector3D Vector3D::abs(const Vector3D &value) { return Vector3D(glm::abs(value.value)); }

Vector3D Vector3D::floor(const Vector3D &value) { return Vector3D(glm::floor(value.value)); }

Vector3D Vector3D::ceil(const Vector3D &value) { return Vector3D(glm::ceil(value.value)); }

Vector3D Vector3D::round(const Vector3D &value) { return Vector3D(glm::round(value.value)); }

Vector3D Vector3D::mod(const Vector3D &a, const Vector3D &b) { return Vector3D(glm::mod(a.value, b.value)); }

Vector3D Vector3D::mod(const Vector3D &a, const float &b) { return Vector3D(glm::mod(a.value, b)); }

Vector3D Vector3D::modf(Vector3D &wholePart, const Vector3D &value) { return Vector3D(glm::modf(value.value, wholePart.value)); }