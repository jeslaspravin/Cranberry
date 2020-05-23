#include "Vector3D.h"
#include "Math.h"

#include <glm/geometric.hpp>

Vector3D::Vector3D(const Matrix3Col& vector3d)
    : value(vector3d)
{}

Vector3D::Vector3D()
    : value(0)
{}

Vector3D::Vector3D(const float& x, const float& y)
    : value(x,y,0)
{}

Vector3D::Vector3D(const float& allValue)
    : value(allValue)
{}

Vector3D::Vector3D(const Vector3D& other)
    : value(other.value)
{}

Vector3D::Vector3D(Vector3D&& other)
    : value(std::move(other.value))
{}

Vector3D::Vector3D(const float& x, const float& y, const float& z)
    : value(x,y,z)
{}

void Vector3D::operator=(const Vector3D & other)
{
    value = other.value;
}

void Vector3D::operator=(Vector3D && other)
{
    value = std::move(other.value);
}

float& Vector3D::x()
{
    return value.x;
}

float Vector3D::x() const
{
    return value.x;
}

float& Vector3D::y()
{
    return value.y;
}

float Vector3D::y() const
{
    return value.y;
}

float& Vector3D::z()
{
    return value.z;
}

float Vector3D::z() const
{
    return value.z;
}

bool Vector3D::operator==(const Vector3D& b) const
{
    return isSame(b);
}

float Vector3D::operator|(const Vector3D& b) const
{
    return glm::dot(value, b.value);
}

Vector3D Vector3D::operator^(const Vector3D& b) const
{
    return Vector3D(glm::cross(value, b.value));
}

Vector3D Vector3D::operator*(const Vector3D& b) const
{
    return Vector3D(value * b.value);
}

void Vector3D::operator*=(const Vector3D& b)
{
    value *= b.value;
}

Vector3D Vector3D::operator*(const float& scalar) const
{
    return Vector3D(value * scalar);
}

void Vector3D::operator*=(const float& scalar)
{
    value *= scalar;
}

Vector3D Vector3D::operator/(const Vector3D& b) const
{
    return Vector3D(value / b.value);
}

void Vector3D::operator/=(const Vector3D& b)
{
    value /= b.value;
}

Vector3D Vector3D::operator/(const float& scalar) const
{
    return Vector3D(value / scalar);
}

void Vector3D::operator/=(const float& scalar)
{
    value /= scalar;
}

Vector3D Vector3D::operator-(const Vector3D& b) const
{
    return Vector3D(value - b.value);
}

void Vector3D::operator-=(const Vector3D& b)
{
    value -= b.value;
}

Vector3D Vector3D::operator-(const float& scalar) const
{
    return Vector3D(value - scalar);
}

void Vector3D::operator-=(const float& scalar)
{
    value -= scalar;
}

Vector3D Vector3D::operator+(const Vector3D& b) const
{
    return Vector3D(value + b.value);
}

void Vector3D::operator+=(const Vector3D& b)
{
    value += b.value;
}

Vector3D Vector3D::operator+(const float& scalar) const
{
    return Vector3D(value + scalar);
}

void Vector3D::operator+=(const float& scalar)
{
    value += scalar;
}

bool Vector3D::isSame(const Vector3D& b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon) 
        && Math::isEqual(value.z, b.value.z, epsilon);
}

Vector3D Vector3D::normalized() const
{
    return Vector3D(glm::normalize(value));
}

Vector3D Vector3D::safeNormalize(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < SMALL_EPSILON)
    {
        return Vector3D::ZERO;
    }
    return Vector3D(value * Math::invSqrt(sqrLen));
}

float Vector3D::length() const
{
    return glm::length(value);
}

float Vector3D::sqrlength() const
{
    glm::vec3 sqrVal = Math::exp2(value);
    return sqrVal.x + sqrVal.y + sqrVal.z;
}

Vector3D Vector3D::projectTo(const Vector3D& b) const
{
    return Vector3D(b * (*this | b) / (b | b));
}

Vector3D Vector3D::rejectFrom(const Vector3D& b) const
{
    return *this - projectTo(b);
}

const Vector3D Vector3D::RIGHT(0, 1);

const Vector3D Vector3D::FWD(1, 0);

const Vector3D Vector3D::UP(0, 0, 1);

const Vector3D Vector3D::ZERO(0);

const Vector3D Vector3D::ONE(1);

float Vector3D::dot(const Vector3D& a, const Vector3D& b)
{
    return a | b;
}

Vector3D Vector3D::cross(const Vector3D& a, const Vector3D& b)
{
    return a ^ b;
}

bool Math::isEqual(const Vector3D& a, const Vector3D& b, float epsilon /*= SMALL_EPSILON*/)
{
    return a.isSame(b, epsilon);
}