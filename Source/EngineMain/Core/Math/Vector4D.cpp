#include "Vector4D.h"
#include "Math.h"
#include "../Platform/PlatformAssertionErrors.h"

#include <glm/geometric.hpp>

Vector4D::Vector4D(const Matrix4Col& vector3d)
    : value(vector3d)
{}

Vector4D::Vector4D()
    : value(0)
{}

Vector4D::Vector4D(const float& allValue)
    : value(allValue)
{}

Vector4D::Vector4D(const Vector4D& other)
    : value(other.value)
{}

Vector4D::Vector4D(Vector4D&& other)
    : value(std::move(other.value))
{}

Vector4D::Vector4D(const float& x, const float& y, const float& z, const float& w)
    : value(x, y, z, w)
{}

void Vector4D::operator=(const Vector4D & other)
{
    value = other.value;
}

void Vector4D::operator=(Vector4D && other)
{
    value = std::move(other.value);
}

float& Vector4D::x()
{
    return value.x;
}

float Vector4D::x() const
{
    return value.x;
}

float& Vector4D::y()
{
    return value.y;
}

float Vector4D::y() const
{
    return value.y;
}

float& Vector4D::z()
{
    return value.z;
}

float Vector4D::z() const
{
    return value.z;
}

float& Vector4D::w()
{
    return value.w;
}

float Vector4D::w() const
{
    return value.w;
}

float Vector4D::operator[](uint32 index) const
{
    debugAssert(index < 4);
    return value[index];
}

float& Vector4D::operator[](uint32 index)
{
    debugAssert(index < 4);
    return value[index];
}

bool Vector4D::operator==(const Vector4D& b) const
{
    return isSame(b);
}

float Vector4D::operator|(const Vector4D& b) const
{
    return glm::dot(value, b.value);
}

Vector4D Vector4D::operator*(const Vector4D& b) const
{
    return Vector4D(value * b.value);
}

void Vector4D::operator*=(const Vector4D& b)
{
    value *= b.value;
}

Vector4D Vector4D::operator*(const float& scalar) const
{
    return Vector4D(value * scalar);
}

void Vector4D::operator*=(const float& scalar)
{
    value *= scalar;
}

Vector4D Vector4D::operator/(const Vector4D& b) const
{
    return Vector4D(value / b.value);
}

void Vector4D::operator/=(const Vector4D& b)
{
    value /= b.value;
}

Vector4D Vector4D::operator/(const float& scalar) const
{
    return Vector4D(value / scalar);
}

void Vector4D::operator/=(const float& scalar)
{
    value /= scalar;
}

Vector4D Vector4D::operator-(const Vector4D& b) const
{
    return Vector4D(value - b.value);
}

void Vector4D::operator-=(const Vector4D& b)
{
    value -= b.value;
}

Vector4D Vector4D::operator-(const float& scalar) const
{
    return Vector4D(value - scalar);
}

void Vector4D::operator-=(const float& scalar)
{
    value -= scalar;
}

Vector4D Vector4D::operator+(const Vector4D& b) const
{
    return Vector4D(value + b.value);
}

void Vector4D::operator+=(const Vector4D& b)
{
    value += b.value;
}

Vector4D Vector4D::operator+(const float& scalar) const
{
    return Vector4D(value + scalar);
}

void Vector4D::operator+=(const float& scalar)
{
    value += scalar;
}

bool Vector4D::isSame(const Vector4D& b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
        && Math::isEqual(value.z, b.value.z, epsilon);
}

Vector4D Vector4D::normalized() const
{
    return Vector4D(glm::normalize(value));
}

Vector4D Vector4D::safeNormalize(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < SMALL_EPSILON)
    {
        return Vector4D::ZERO;
    }
    return Vector4D(value * Math::invSqrt(sqrLen));
}

float Vector4D::length() const
{
    return glm::length(value);
}

float Vector4D::sqrlength() const
{
    return Math::pow2(value.x) + Math::pow2(value.y) + Math::pow2(value.z) + Math::pow2(value.w);
}

Vector4D Vector4D::projectTo(const Vector4D& b) const
{
    return Vector4D(b * (*this | b) / (b | b));
}

Vector4D Vector4D::rejectFrom(const Vector4D& b) const
{
    return *this - projectTo(b);
}

const Vector4D Vector4D::ZERO(0);
const Vector4D Vector4D::ONE(1);

FORCE_INLINE float Vector4D::dot(const Vector4D& a, const Vector4D& b)
{
    return a | b;
}

FORCE_INLINE Vector4D Vector4D::clamp(const Vector4D& value, const Vector4D& min, const Vector4D& max)
{
    return Vector4D(glm::clamp(value.value, min.value, max.value));
}

FORCE_INLINE Vector4D Vector4D::min(const Vector4D& a, const Vector4D& b)
{
    return Vector4D(glm::min(a.value, b.value));
}

FORCE_INLINE Vector4D Vector4D::max(const Vector4D& a, const Vector4D& b)
{
    return Vector4D(glm::max(a.value, b.value));
}

FORCE_INLINE Vector4D Vector4D::abs(const Vector4D& value)
{
    return Vector4D(glm::abs(value.value));
}

FORCE_INLINE Vector4D Vector4D::floor(const Vector4D& value)
{
    return Vector4D(glm::floor(value.value));
}

FORCE_INLINE Vector4D Vector4D::ceil(const Vector4D& value)
{
    return Vector4D(glm::ceil(value.value));
}

//////////////////////////////////////////////////////////////////////////
// Math functions
//////////////////////////////////////////////////////////////////////////

bool Math::isEqual(const Vector4D& a, const Vector4D& b, float epsilon /*= SMALL_EPSILON*/)
{
    return a.isSame(b, epsilon);
}

Vector4D Math::clamp(const Vector4D& value, const Vector4D& min, const Vector4D& max)
{
    return Vector4D::clamp(value, min, max);
}

Vector4D Math::min(const Vector4D& a, const Vector4D& b)
{
    return Vector4D::min(a, b);
}

Vector4D Math::max(const Vector4D& a, const Vector4D& b)
{
    return Vector4D::max(a, b);
}

Vector4D Math::abs(const Vector4D& value)
{
    return Vector4D::abs(value);
}

Vector4D Math::floor(const Vector4D& value)
{
    return Vector4D::floor(value);
}

Vector4D Math::ceil(const Vector4D& value)
{
    return Vector4D::ceil(value);
}