#include "Rotation.h"
#include "Math.h"

Rotation::Rotation(const glm::vec3& rotValue)
    : value(rotValue)
{}

Rotation::Rotation()
    : value(0)
{}

Rotation::Rotation(const float& allValue)
    : value(allValue)
{}

Rotation::Rotation(const Rotation& other)
    : value(other.value)
{}

Rotation::Rotation(Rotation&& other)
    : value(std::move(other.value))
{}

Rotation::Rotation(const float& r, const float& p, const float& y)
    : value(r, p, y)
{}

void Rotation::operator=(const Rotation& other)
{
    value = other.value;
}

void Rotation::operator=(Rotation&& other)
{
    value = std::move(other.value);
}

float& Rotation::roll()
{
    return value.x;
}

float Rotation::roll() const
{
    return value.x;
}

float& Rotation::pitch()
{
    return value.y;
}

float Rotation::pitch() const
{
    return value.y;
}

float& Rotation::yaw()
{
    return value.z;
}

float Rotation::yaw() const
{
    return value.z;
}

bool Rotation::operator==(const Rotation& b) const
{
    return isSame(b);
}

Rotation Rotation::operator*(const Rotation& b) const
{
    return Rotation(value * b.value);
}

void Rotation::operator*=(const Rotation& b)
{
    value *= b.value;
}

Rotation Rotation::operator*(const float& scalar) const
{
    return Rotation(value * scalar);
}

void Rotation::operator*=(const float& scalar)
{
    value *= scalar;
}

Rotation Rotation::operator/(const Rotation& b) const
{
    return Rotation(value / b.value);
}

void Rotation::operator/=(const Rotation& b)
{
    value /= b.value;
}

Rotation Rotation::operator/(const float& scalar) const
{
    return Rotation(value / scalar);
}

void Rotation::operator/=(const float& scalar)
{
    value /= scalar;
}

Rotation Rotation::operator-(const Rotation& b) const
{
    return Rotation(value - b.value);
}

void Rotation::operator-=(const Rotation& b)
{
    value -= b.value;
}

Rotation Rotation::operator-(const float& scalar) const
{
    return Rotation(value - scalar);
}

void Rotation::operator-=(const float& scalar)
{
    value -= scalar;
}

Rotation Rotation::operator+(const Rotation& b) const
{
    return Rotation(value + b.value);
}

void Rotation::operator+=(const Rotation& b)
{
    value += b.value;
}

Rotation Rotation::operator+(const float& scalar) const
{
    return Rotation(value + scalar);
}

void Rotation::operator+=(const float& scalar)
{
    value += scalar;
}

bool Rotation::isSame(const Rotation& b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
        && Math::isEqual(value.z, b.value.z, epsilon);
}

bool Math::isEqual(const Rotation& a, const Rotation& b, float epsilon /*= SMALL_EPSILON*/)
{
    return a.isSame(b, epsilon);
}

Rotation Math::deg2Rad(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::deg2Rad(rotVal);
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::rad2Deg(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::rad2Deg(rotVal);
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::sin(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::sin(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::cos(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::cos(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::tan(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::tan(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::asin(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::asin(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::acos(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::acos(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::atan(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::atan(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}
