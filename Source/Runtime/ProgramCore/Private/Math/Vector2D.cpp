#include "Math/Vector2D.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Math/Math.h"
#include "Math/Vector3D.h"

#include <glm/geometric.hpp>
#include <glm/ext/vector_float3.hpp>

Vector2D::Vector2D(const Matrix2Col& vector2d)
    : value(vector2d)
{}

Vector2D::Vector2D()
    : value(0)
{}

Vector2D::Vector2D(const float& x, const float& y)
    : value(x,y)
{}

Vector2D::Vector2D(const Vector2D& other)
    : value(other.value)
{}

Vector2D::Vector2D(Vector2D&& other)
    : value(std::move(other.value))
{}

Vector2D::Vector2D(const float& allValue)
    : value(allValue)
{}

Vector2D::Vector2D(const Vector3D& other)
    : value(other.x(), other.y())
{}

void Vector2D::operator=(const Vector2D & other)
{
    value = other.value;
}

void Vector2D::operator=(Vector2D && other)
{
    value = std::move(other.value);
}

float& Vector2D::x()
{
    return value.x;
}

float Vector2D::x() const
{
    return value.x;
}

float& Vector2D::y()
{
    return value.y;
}

float Vector2D::y() const
{
    return value.y;
}

float Vector2D::operator[](uint32 index) const
{
    debugAssert(index < 2);
    return value[index];
}

float& Vector2D::operator[](uint32 index)
{
    debugAssert(index < 2);
    return value[index];
}

bool Vector2D::operator==(const Vector2D& b) const
{
    return isSame(b);
}

float Vector2D::operator|(const Vector2D& b) const
{
    return glm::dot(value, b.value);
}

float Vector2D::operator^(const Vector2D& b) const
{
    return glm::cross(glm::vec3(value,0), glm::vec3(b.value,0)).z;
}

Vector2D Vector2D::operator*(const Vector2D& b) const
{
    return Vector2D(value * b.value);
}

void Vector2D::operator*=(const Vector2D& b)
{
    value *= b.value;
}

Vector2D Vector2D::operator*(const float& scalar) const
{
    return Vector2D(value * scalar);
}

void Vector2D::operator*=(const float& scalar)
{
    value *= scalar;
}

Vector2D Vector2D::operator/(const Vector2D& b) const
{
    return Vector2D(value / b.value);
}

void Vector2D::operator/=(const Vector2D& b)
{
    value /= b.value;
}

Vector2D Vector2D::operator/(const float& scalar) const
{
    return Vector2D(value / scalar);
}

void Vector2D::operator/=(const float& scalar)
{
    value /= scalar;
}

Vector2D Vector2D::operator-(const Vector2D& b) const
{
    return Vector2D(value - b.value);
}

void Vector2D::operator-=(const Vector2D& b)
{
    value -= b.value;
}

Vector2D Vector2D::operator-(const float& scalar) const
{
    return Vector2D(value - scalar);
}

void Vector2D::operator-=(const float& scalar)
{
    value -= scalar;
}

Vector2D Vector2D::operator-() const
{
    return Vector2D(-value);
}

Vector2D Vector2D::operator+(const Vector2D& b) const
{
    return Vector2D(value + b.value);
}

void Vector2D::operator+=(const Vector2D& b)
{
    value += b.value;
}

Vector2D Vector2D::operator+(const float& scalar) const
{
    return Vector2D(value + scalar);
}

void Vector2D::operator+=(const float& scalar)
{
    value += scalar;
}

bool Vector2D::isSame(const Vector2D& b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon);
}

Vector2D Vector2D::normalized() const
{
    return Vector2D(glm::normalize(value));
}

Vector2D Vector2D::safeNormalize(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < SMALL_EPSILON)
    {
        return Vector2D::ZERO;
    }
    return Vector2D(value * Math::invSqrt(sqrLen));
}

float Vector2D::length() const
{
    return glm::length(value);
}

float Vector2D::sqrlength() const
{
    return Math::pow2(value.x) + Math::pow2(value.y);
}

Vector2D Vector2D::projectTo(const Vector2D& b) const
{
    return Vector2D(b * (*this | b) / (b | b));
}

Vector2D Vector2D::rejectFrom(const Vector2D& b) const
{
    return *this - projectTo(b);
}

const Vector2D Vector2D::RIGHT(0,1);

const Vector2D Vector2D::FWD(1,0);

const Vector2D Vector2D::ZERO(0);

const Vector2D Vector2D::ONE(1);

float Vector2D::dot(const Vector2D& a, const Vector2D& b)
{
    return a | b;
}

float Vector2D::cross(const Vector2D& a, const Vector2D& b)
{
    return a ^ b;
}

Vector2D Vector2D::clamp(const Vector2D& value, const Vector2D& min, const Vector2D& max)
{
    return Vector2D(glm::clamp(value.value, min.value, max.value));
}

Vector2D Vector2D::min(const Vector2D& a, const Vector2D& b)
{
    return Vector2D(glm::min(a.value, b.value));
}

Vector2D Vector2D::max(const Vector2D& a, const Vector2D& b)
{
    return Vector2D(glm::max(a.value, b.value));
}

Vector2D Vector2D::abs(const Vector2D& value)
{
    return Vector2D(glm::abs(value.value));
}

Vector2D Vector2D::floor(const Vector2D& value)
{
    return Vector2D(glm::floor(value.value));
}

Vector2D Vector2D::ceil(const Vector2D& value)
{
    return Vector2D(glm::ceil(value.value));
}

Vector2D Vector2D::round(const Vector2D& value)
{
    return Vector2D(glm::round(value.value));
}

Vector2D Vector2D::mod(const Vector2D& a, const Vector2D& b)
{
    return Vector2D(glm::mod(a.value, b.value));
}

Vector2D Vector2D::mod(const Vector2D& a, const float& b)
{
    return Vector2D(glm::mod(a.value, b));
}

Vector2D Vector2D::modf(Vector2D& wholePart, const Vector2D& value)
{
    return Vector2D(glm::modf(value.value, wholePart.value));
}