#include "Math/Matrix3.h"
#include "Math/Vector3D.h"


Matrix3::Matrix3(const glm::mat3& matrix)
    : value(matrix)
{}

Matrix3::Matrix3()
    : value(0)
{}

Matrix3::Matrix3(float allValue)
    : value(allValue)
{}

Matrix3::Matrix3(const Vector3D& c1, const Vector3D& c2, const Vector3D& c3)
    : value(c1.x(), c1.y(),c1.z() ,c2.x(), c2.y(), c2.z(),c3.x(),c3.y(),c3.z())
{}

Matrix3::Matrix3(const float& c1x, const float& c1y, const float& c1z
    , const float& c2x, const float& c2y, const float& c2z
    , const float& c3x, const float& c3y, const float& c3z)
    : value(c1x, c1y, c1z, c2x, c2y, c2z, c3x, c3y, c3z)
{}

Matrix3::Matrix3(const Matrix3& other)
    : value(other.value)
{}

Matrix3::Matrix3(Matrix3&& other)
    : value(std::move(other.value))
{}

Matrix3::Matrix3(const Vector3D& scale)
    : value(
          scale.x(), 0, 0
        , 0, scale.y(), 0
        , 0, 0, scale.z())
{}

void Matrix3::operator=(const Matrix3& other)
{
    value = other.value;
}

void Matrix3::operator=(Matrix3&& other)
{
    value = std::move(other.value);
}

Matrix3Col& Matrix3::operator[](uint32 colIndex)
{
    return value[colIndex];
}

Matrix3Col Matrix3::operator[](uint32 colIndex) const
{
    return value[colIndex];
}

Vector3D Matrix3::operator*(const Vector3D& transformingVector) const
{
    return Vector3D(value * transformingVector.value);
}

Matrix3 Matrix3::operator*(const Matrix3& b) const
{
    return value * b.value;
}

void Matrix3::operator*=(const Matrix3& b)
{
    value *= b.value;
}

Matrix3 Matrix3::operator*(const float& scalar) const
{
    return value * scalar;
}

void Matrix3::operator*=(const float& scalar)
{
    value *= scalar;
}

Matrix3 Matrix3::inverse() const
{
    return glm::inverse(value);
}

float Matrix3::determinant() const
{
    return glm::determinant(value);
}

Matrix3 Matrix3::transpose() const
{
    return glm::transpose(value);
}

Matrix3 Matrix3::operator|(const Matrix3& b) const
{
    return glm::matrixCompMult(value, b.value);
}

void Matrix3::operator|=(const Matrix3& b)
{
    value = glm::matrixCompMult(value, b.value);
}

Matrix3 Matrix3::operator/(const Matrix3& b) const
{
    return value / b.value;
}

void Matrix3::operator/=(const Matrix3& b)
{
    value /= b.value;
}

Matrix3 Matrix3::operator/(const float& scalar) const
{
    return Matrix3(value / scalar);
}

void Matrix3::operator/=(const float& scalar)
{
    value /= scalar;
}

Matrix3 Matrix3::operator-(const Matrix3& b) const
{
    return (value - b.value);
}

void Matrix3::operator-=(const Matrix3& b)
{
    value -= b.value;
}

Matrix3 Matrix3::operator-(const float& scalar) const
{
    return (value - scalar);
}

void Matrix3::operator-=(const float& scalar)
{
    value -= scalar;
}

Matrix3 Matrix3::operator-() const
{
    return -value;
}

Matrix3 Matrix3::operator+(const Matrix3& b) const
{
    return (value + b.value);
}

void Matrix3::operator+=(const Matrix3& b)
{
    value += b.value;
}

Matrix3 Matrix3::operator+(const float& scalar) const
{
    return (value + scalar);
}

void Matrix3::operator+=(const float& scalar)
{
    value += scalar;
}

const Matrix3 Matrix3::IDENTITY{ Vector3D(1, 0), Vector3D(0, 1), Vector3D(0,0,1) };
