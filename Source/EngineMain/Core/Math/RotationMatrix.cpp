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
