#include "Vector2D.h"
#include "Math.h"

#include <glm/geometric.hpp>
#include <glm/ext/vector_float3.hpp>

Vector2D::Vector2D(const glm::vec2& vector2d)
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

float& Vector2D::x()
{
    return value.x;
}

float Vector2D::x() const
{
    return value.y;
}

float& Vector2D::y()
{
    return value.x;
}

float Vector2D::y() const
{
    return value.y;
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

bool Vector2D::isSame(const Vector2D& b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon);
}

Vector2D Vector2D::normalized()
{
    return glm::normalize(value);
}

Vector2D Vector2D::safeNormalize(float threshold /*= SMALL_EPSILON*/)
{
    float sqrLen = sqrlength();
    if (sqrLen < SMALL_EPSILON)
    {
        return Vector2D::ZERO;
    }
    return value * Math::invSqrt(sqrLen);
}

float Vector2D::length()
{
    return glm::length(value);
}

float Vector2D::sqrlength()
{
    glm::vec2 sqrVal = Math::exp2(value);
    return sqrVal.x + sqrVal.y;
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

bool Math::isEqual(const Vector2D& a, const Vector2D& b, float epsilon /*= SMALL_EPSILON*/)
{
    return a.isSame(b,epsilon);
}
