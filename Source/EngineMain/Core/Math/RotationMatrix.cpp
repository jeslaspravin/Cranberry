#include "RotationMatrix.h"
#include "Rotation.h"
#include "Vector3D.h"
#include "Math.h"

RotationMatrix::RotationMatrix()
    : rotationMatrix(Vector3D::FWD, Vector3D::RIGHT, Vector3D::UP)
{}

RotationMatrix::RotationMatrix(const class Rotation& rotation)
{
    Rotation cosVal = Math::cos(rotation);
    Rotation sinVal = Math::sin(rotation);

    rotationMatrix[0].x = cosVal.yaw() * cosVal.pitch();
    rotationMatrix[0].y = sinVal.yaw() * cosVal.roll() + cosVal.yaw() * sinVal.pitch() * sinVal.roll();
    rotationMatrix[0].z = sinVal.yaw() * sinVal.roll() - cosVal.yaw() * sinVal.pitch() * cosVal.roll();

    rotationMatrix[1].x = -sinVal.yaw() * cosVal.pitch();
    rotationMatrix[1].y = cosVal.yaw() * cosVal.roll() - sinVal.yaw() * sinVal.pitch() * sinVal.roll();
    rotationMatrix[1].z = cosVal.yaw() * sinVal.roll() + sinVal.yaw() * sinVal.pitch() * cosVal.roll();

    rotationMatrix[2].x = sinVal.pitch();
    rotationMatrix[2].y = -cosVal.pitch() * sinVal.roll();
    rotationMatrix[2].z = cosVal.pitch() * cosVal.roll();
}

RotationMatrix::RotationMatrix(const Matrix3& rotMatrix)
    : rotationMatrix(rotMatrix)
{}

RotationMatrix::RotationMatrix(const Matrix4& rotMatrix)
    : rotationMatrix(rotMatrix[0].x, rotMatrix[0].y, rotMatrix[0].z
        , rotMatrix[1].x, rotMatrix[1].y, rotMatrix[1].z
        , rotMatrix[2].x, rotMatrix[2].y, rotMatrix[2].z)
{}

RotationMatrix::RotationMatrix(const RotationMatrix& other)
    : rotationMatrix(other.rotationMatrix)
{}

RotationMatrix::RotationMatrix(RotationMatrix&& other)
    : rotationMatrix(std::move(other.rotationMatrix))
{}

Rotation RotationMatrix::asRotation() const
{
    glm::vec3 numerator(rotationMatrix[2][1], rotationMatrix[2][0], rotationMatrix[1][0]);
    glm::vec3 denominator(rotationMatrix[2][2], Math::sqrt(rotationMatrix[2][1] * rotationMatrix[2][1] + rotationMatrix[2][2] * rotationMatrix[2][2])
        , rotationMatrix[0][0]);

    numerator = Math::rad2Deg(Math::atan(numerator, denominator));
    return Rotation(-numerator.x, numerator.y, -numerator.z);
}

const Matrix3& RotationMatrix::matrix() const
{
    return rotationMatrix;
}

void RotationMatrix::orthogonalize()
{
    Vector3D x(rotationMatrix[0]);
    Vector3D y(rotationMatrix[1]);
    Vector3D z(rotationMatrix[2]);

    y = y.rejectFrom(x).safeNormalize();
    z = z.rejectFrom(y).rejectFrom(x).safeNormalize();
    rotationMatrix = Matrix3(x.safeNormalize(), y, z);
}

RotationMatrix RotationMatrix::fromX(const Vector3D& x)
{
    // Assuming z up
    Vector3D normX = x.safeNormalize();

    // If X Parallel to Z then consider y right
    if (Math::isEqual(Math::abs(normX | Vector3D::UP), 1.f))
    {
        return RotationMatrix(Matrix3(normX, Vector3D::RIGHT, normX ^ Vector3D::RIGHT));
    }
    return RotationMatrix(Matrix3(normX, Vector3D::UP ^ normX, Vector3D::UP));
}

RotationMatrix RotationMatrix::fromY(const Vector3D& y)
{
    // Assuming z up
    Vector3D normY = y.safeNormalize();

    // If Y Parallel to Z then consider x fwd
    if (Math::isEqual(Math::abs(normY | Vector3D::UP), 1.f))
    {
        return RotationMatrix(Matrix3(Vector3D::FWD, normY, Vector3D::FWD ^ normY));
    }
    return RotationMatrix(Matrix3(normY ^ Vector3D::UP, normY, Vector3D::UP));
}

RotationMatrix RotationMatrix::fromZ(const Vector3D& z)
{
    // Assuming x forward
    Vector3D normZ = z.safeNormalize();
    // If Z Parallel to X then consider y right
    if (Math::isEqual(Math::abs(normZ | Vector3D::FWD), 1.f))
    {
        return RotationMatrix(Matrix3(Vector3D::RIGHT ^ normZ, Vector3D::RIGHT, normZ));
    }
    return RotationMatrix(Matrix3(Vector3D::FWD, normZ ^ Vector3D::FWD, normZ));
}

RotationMatrix RotationMatrix::fromXY(const Vector3D& x, const Vector3D& y)
{
    Vector3D normX = x.safeNormalize();
    Vector3D normY = y.rejectFrom(normX).safeNormalize();

    return RotationMatrix(Matrix3(normX, normY, normX ^ normY));
}

RotationMatrix RotationMatrix::fromYZ(const Vector3D& y, const Vector3D& z)
{
    Vector3D normZ = z.safeNormalize();
    Vector3D normY = y.rejectFrom(normZ).safeNormalize();

    return RotationMatrix(Matrix3(normY ^ normZ, normY, normZ));
}

RotationMatrix RotationMatrix::fromZX(const Vector3D& z, const Vector3D& x)
{
    Vector3D normX = x.safeNormalize();
    Vector3D normZ = z.rejectFrom(normX).safeNormalize();

    return RotationMatrix(Matrix3(normX, normZ ^ normX, normZ));
}

RotationMatrix RotationMatrix::fromXYZ(const Vector3D& x, const Vector3D& y, const Vector3D& z)
{
    RotationMatrix retVal(Matrix3(x.safeNormalize(), y, z));
    retVal.orthogonalize();
    return retVal;
}
