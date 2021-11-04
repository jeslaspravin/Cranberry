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
    rotVal = Math::rad2Deg(Math::asin(Math::deg2Rad(rotVal)));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::acos(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::rad2Deg(Math::acos(Math::deg2Rad(rotVal)));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::atan(const Rotation& value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::rad2Deg(Math::atan(Math::deg2Rad(rotVal)));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Rotation::clamp(const Rotation& value, const Rotation& min, const Rotation& max)
{
    return Rotation(glm::clamp(value.value, min.value, max.value));
}

Rotation Rotation::min(const Rotation& a, const Rotation& b)
{
    return Rotation(glm::min(a.value, b.value));
}

Rotation Rotation::max(const Rotation& a, const Rotation& b)
{
    return Rotation(glm::max(a.value, b.value));
}

Rotation Rotation::abs(const Rotation& value)
{
    return Rotation(glm::abs(value.value));
}

Rotation Rotation::floor(const Rotation& value)
{
    return Rotation(glm::floor(value.value));
}

Rotation Rotation::ceil(const Rotation& value)
{
    return Rotation(glm::ceil(value.value));
}

Rotation Rotation::round(const Rotation& value)
{
    return Rotation(glm::round(value.value));
}

Rotation Rotation::mod(const Rotation& a, const Rotation& b)
{
    return Rotation(glm::mod(a.value, b.value));
}

Rotation Rotation::mod(const Rotation& a, const float& b)
{
    return Rotation(glm::mod(a.value, b));
}

Rotation Rotation::modf(Rotation& wholePart, const Rotation& value)
{
    return Rotation(glm::modf(value.value, wholePart.value));
}
