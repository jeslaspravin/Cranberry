#include "Matrix4.h"
#include "Vector3D.h"
#include "Vector4D.h"


Matrix4::Matrix4(const glm::mat4& matrix)
    : value(matrix)
{}

Matrix4::Matrix4()
    : value(0)
{}

Matrix4::Matrix4(float allValue)
    : value(allValue)
{}

Matrix4::Matrix4(const Vector3D& c1, const Vector3D& c2, const Vector3D& c3, const Vector3D& c4, const float& c4w/*=1.0f*/)
    : value(c1.x(), c1.y(), c1.z(), 0, c2.x(), c2.y(), c2.z(), 0, c3.x(), c3.y(), c3.z(), 0, c4.x(), c4.y(), c4.z(), c4w)
{}

Matrix4::Matrix4(const Matrix4& other)
    : value(other.value)
{}

Matrix4::Matrix4(Matrix4&& other)
    : value(std::move(other.value))
{}

Matrix4::Matrix4(const Vector4D& c1, const Vector4D& c2, const Vector4D& c3, const Vector4D& c4)
    : value(c1.x(), c1.y(), c1.z(), c1.w(), c2.x(), c2.y(), c2.z(), c2.w(), c3.x(), c3.y(), c3.z(), c3.w(), c4.x(), c4.y(), c4.z(), c4.w())
{}

Matrix4::Matrix4(const float& c1x, const float& c1y, const float& c1z, const float& c1w
    , const float& c2x, const float& c2y, const float& c2z, const float& c2w
    , const float& c3x, const float& c3y, const float& c3z, const float& c3w
    , const float& c4x, const float& c4y, const float& c4z, const float& c4w)
    : value(
          c1x, c1y, c1z, c1w
        , c2x, c2y, c2z, c2w
        , c3x, c3y, c3z, c3w
        , c4x, c4y, c4z, c4w)
{}

Matrix4::Matrix4(const Vector3D& scale)
    : value(
          scale.x(), scale.y(), scale.z(), 0
        , scale.x(), scale.y(), scale.z(), 0
        , scale.x(), scale.y(), scale.z(), 0
        , 0, 0, 0, 1)
{}

void Matrix4::operator=(const Matrix4 & other)
{
    value = other.value;
}

void Matrix4::operator=(Matrix4 && other)
{
    value = std::move(other.value);
}

Matrix4Col& Matrix4::operator[](uint32 colIndex)
{
    return value[colIndex];
}

Matrix4Col Matrix4::operator[](uint32 colIndex) const
{
    return value[colIndex];
}

Vector4D Matrix4::operator*(const Vector4D& transformingVector) const
{
    return Vector4D(value * transformingVector.value);
}

Vector3D Matrix4::operator*(const Vector3D& transformingVector) const
{
    Vector4D vector4D(transformingVector.x(),transformingVector.y(),transformingVector.z(), 1.0f);
    vector4D = (*this) * vector4D;
    return Vector3D(vector4D.x(),vector4D.y(),vector4D.z());
}

Matrix4 Matrix4::operator*(const Matrix4& b) const
{
    return value * b.value;
}

void Matrix4::operator*=(const Matrix4& b)
{
    value *= b.value;
}

Matrix4 Matrix4::operator*(const float& scalar) const
{
    return value * scalar;
}

void Matrix4::operator*=(const float& scalar)
{
    value *= scalar;
}

Matrix4 Matrix4::inverse() const
{
    return glm::inverse(value);
}

float Matrix4::determinant() const
{
    return glm::determinant(value);
}

Matrix4 Matrix4::transpost() const
{
    return glm::transpose(value);
}

Matrix4 Matrix4::operator|(const Matrix4& b) const
{
    return glm::matrixCompMult(value, b.value);
}

void Matrix4::operator|=(const Matrix4& b)
{
    value = glm::matrixCompMult(value, b.value);
}

Matrix4 Matrix4::operator/(const Matrix4& b) const
{
    return value / b.value;
}

void Matrix4::operator/=(const Matrix4& b)
{
    value /= b.value;
}

Matrix4 Matrix4::operator/(const float& scalar) const
{
    return Matrix4(value / scalar);
}

void Matrix4::operator/=(const float& scalar)
{
    value /= scalar;
}

Matrix4 Matrix4::operator-(const Matrix4& b) const
{
    return (value - b.value);
}

void Matrix4::operator-=(const Matrix4& b)
{
    value -= b.value;
}

Matrix4 Matrix4::operator-(const float& scalar) const
{
    return (value - scalar);
}

void Matrix4::operator-=(const float& scalar)
{
    value -= scalar;
}

Matrix4 Matrix4::operator+(const Matrix4& b) const
{
    return (value + b.value);
}

void Matrix4::operator+=(const Matrix4& b)
{
    value += b.value;
}

Matrix4 Matrix4::operator+(const float& scalar) const
{
    return (value + scalar);
}

void Matrix4::operator+=(const float& scalar)
{
    value += scalar;
}

const Matrix4 Matrix4::IDENTITY{ Vector3D::FWD, Vector3D::RIGHT, Vector3D::UP, Vector3D::ZERO };
