/*!
 * \file RotationMatrix.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/RotationMatrix.h"
#include "Math/Math.h"
#include "Math/Rotation.h"
#include "Math/Vector3.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#define ROTATIONORDER_RPY 1 // Yaw on top of Pitch on top of Roll
#define ROTATIONORDER_YPR 0
FORCE_INLINE void RotationMatrix::fromRotation(const class Rotation &rotation)
{
    Rotation cosVal = Math::cos(rotation);
    Rotation sinVal = Math::sin(rotation);

#if ROTATIONORDER_YPR
    rotationMatrix[0].x = cosVal.yaw() * cosVal.pitch();
    rotationMatrix[0].y = sinVal.yaw() * cosVal.roll() + cosVal.yaw() * sinVal.pitch() * sinVal.roll();
    rotationMatrix[0].z = sinVal.yaw() * sinVal.roll() - cosVal.yaw() * sinVal.pitch() * cosVal.roll();

    rotationMatrix[1].x = -sinVal.yaw() * cosVal.pitch();
    rotationMatrix[1].y = cosVal.yaw() * cosVal.roll() - sinVal.yaw() * sinVal.pitch() * sinVal.roll();
    rotationMatrix[1].z = cosVal.yaw() * sinVal.roll() + sinVal.yaw() * sinVal.pitch() * cosVal.roll();

    rotationMatrix[2].x = sinVal.pitch();
    rotationMatrix[2].y = -cosVal.pitch() * sinVal.roll();
    rotationMatrix[2].z = cosVal.pitch() * cosVal.roll();
#elif ROTATIONORDER_RPY
    rotationMatrix[0].x = cosVal.yaw() * cosVal.pitch();
    rotationMatrix[0].y = sinVal.yaw() * cosVal.pitch();
    rotationMatrix[0].z = -sinVal.pitch();

    rotationMatrix[1].x = cosVal.yaw() * sinVal.pitch() * sinVal.roll() - sinVal.yaw() * cosVal.roll();
    rotationMatrix[1].y = sinVal.yaw() * sinVal.pitch() * sinVal.roll() + cosVal.yaw() * cosVal.roll();
    rotationMatrix[1].z = cosVal.pitch() * sinVal.roll();

    rotationMatrix[2].x = cosVal.yaw() * sinVal.pitch() * cosVal.roll() + sinVal.yaw() * sinVal.roll();
    rotationMatrix[2].y = sinVal.yaw() * sinVal.pitch() * cosVal.roll() - cosVal.yaw() * sinVal.roll();
    rotationMatrix[2].z = cosVal.pitch() * cosVal.roll();
#endif
}

/* Related Rotation Impl */
Vector3 Rotation::fwdVector() const
{
    Rotation cosVal = Math::cos(*this);
    Rotation sinVal = Math::sin(*this);

    Vector3 fwd;
#if ROTATIONORDER_YPR
    fwd[0] = cosVal.yaw() * cosVal.pitch();
    fwd[1] = sinVal.yaw() * cosVal.roll() + cosVal.yaw() * sinVal.pitch() * sinVal.roll();
    fwd[2] = sinVal.yaw() * sinVal.roll() - cosVal.yaw() * sinVal.pitch() * cosVal.roll();
#elif ROTATIONORDER_RPY
    fwd[0] = cosVal.yaw() * cosVal.pitch();
    fwd[1] = sinVal.yaw() * cosVal.pitch();
    fwd[2] = -sinVal.pitch();
#endif
    return fwd;
}

Vector3 Rotation::rightVector() const
{
    Rotation cosVal = Math::cos(*this);
    Rotation sinVal = Math::sin(*this);

    Vector3 right;
#if ROTATIONORDER_YPR
    right[0] = -sinVal.yaw() * cosVal.pitch();
    right[1] = cosVal.yaw() * cosVal.roll() - sinVal.yaw() * sinVal.pitch() * sinVal.roll();
    right[2] = cosVal.yaw() * sinVal.roll() + sinVal.yaw() * sinVal.pitch() * cosVal.roll();
#elif ROTATIONORDER_RPY
    right[0] = cosVal.yaw() * sinVal.pitch() * sinVal.roll() - sinVal.yaw() * cosVal.roll();
    right[1] = sinVal.yaw() * sinVal.pitch() * sinVal.roll() + cosVal.yaw() * cosVal.roll();
    right[2] = cosVal.pitch() * sinVal.roll();
#endif
    return right;
}

Vector3 Rotation::upVector() const
{
    Rotation cosVal = Math::cos(*this);
    Rotation sinVal = Math::sin(*this);

    Vector3 up;
#if ROTATIONORDER_YPR
    up[0] = sinVal.pitch();
    up[1] = -cosVal.pitch() * sinVal.roll();
    up[2] = cosVal.pitch() * cosVal.roll();
#elif ROTATIONORDER_RPY
    up[0] = cosVal.yaw() * sinVal.pitch() * cosVal.roll() + sinVal.yaw() * sinVal.roll();
    up[1] = sinVal.yaw() * sinVal.pitch() * cosVal.roll() - cosVal.yaw() * sinVal.roll();
    up[2] = cosVal.pitch() * cosVal.roll();
#endif
    return up;
}

Rotation RotationMatrix::asRotation() const
{
#if ROTATIONORDER_YPR
    glm::vec3 numerator(-rotationMatrix[2][1], rotationMatrix[2][0], -rotationMatrix[1][0]);
    glm::vec3 denominator(
        rotationMatrix[2][2], Math::sqrt(rotationMatrix[2][1] * rotationMatrix[2][1] + rotationMatrix[2][2] * rotationMatrix[2][2]),
        rotationMatrix[0][0]
    );

#elif ROTATIONORDER_RPY
    glm::vec3 numerator(rotationMatrix[1][2], -rotationMatrix[0][2], rotationMatrix[0][1]);
    glm::vec3 denominator(
        rotationMatrix[2][2], Math::sqrt(rotationMatrix[1][2] * rotationMatrix[1][2] + rotationMatrix[2][2] * rotationMatrix[2][2]),
        rotationMatrix[0][0]
    );

#endif
    numerator = Math::rad2Deg(Math::atan(numerator, denominator));
    return Rotation(numerator.x, numerator.y, numerator.z);
}
#undef ROTATIONORDER_RPY
#undef ROTATIONORDER_YPR

RotationMatrix::RotationMatrix()
    : rotationMatrix(Vector3::FWD, Vector3::RIGHT, Vector3::UP)
{}

RotationMatrix::RotationMatrix(const class Rotation &rotation) { fromRotation(rotation); }

RotationMatrix::RotationMatrix(const Matrix3 &rotMatrix)
    : rotationMatrix(rotMatrix)
{
    verifyMatrix();
}

RotationMatrix::RotationMatrix(const Matrix4 &rotMatrix)
    : rotationMatrix(
        rotMatrix[0].x, rotMatrix[0].y, rotMatrix[0].z, rotMatrix[1].x, rotMatrix[1].y, rotMatrix[1].z, rotMatrix[2].x, rotMatrix[2].y,
        rotMatrix[2].z
    )
{
    verifyMatrix();
}

RotationMatrix::RotationMatrix(const RotationMatrix &other)
    : rotationMatrix(other.rotationMatrix)
{}

RotationMatrix::RotationMatrix(RotationMatrix &&other)
    : rotationMatrix(std::move(other.rotationMatrix))
{}

void RotationMatrix::verifyMatrix() const
{
    debugAssertf(
        Math::isEqual(Vector3(rotationMatrix[0]).sqrlength(), 1.0f, SLIGHTLY_SMALL_EPSILON)
            && Math::isEqual(Vector3(rotationMatrix[1]).sqrlength(), 1.0f, SLIGHTLY_SMALL_EPSILON)
            && Math::isEqual(Vector3(rotationMatrix[2]).sqrlength(), 1.0f, SLIGHTLY_SMALL_EPSILON),
        "Matrix4's rotation matrix must be orthogonal"
    );
}

const Matrix3 &RotationMatrix::matrix() const { return rotationMatrix; }

void RotationMatrix::orthogonalize()
{
    Vector3 x(rotationMatrix[0]);
    Vector3 y(rotationMatrix[1]);
    Vector3 z(rotationMatrix[2]);

    // Gram-Schmidt orthogonalize
    y = y.rejectFrom(x).safeNormalized();
    z = z.rejectFrom(y).rejectFrom(x).safeNormalized();
    rotationMatrix = Matrix3(x.safeNormalized(), y, z);
}

RotationMatrix RotationMatrix::fromX(const Vector3 &x)
{
    // Assuming z up
    Vector3 normX = x.safeNormalized();

    // If X Parallel to Z then consider y right
    if (Math::isEqual(Math::abs(normX | Vector3::UP), 1.f, SLIGHTLY_SMALL_EPSILON))
    {
        Vector3 normZ = (normX ^ Vector3::RIGHT).safeNormalized();
        return RotationMatrix(Matrix3(normX, normZ ^ normX, normZ));
    }
    Vector3 normY = (Vector3::UP ^ normX).safeNormalized();
    return RotationMatrix(Matrix3(normX, normY, normX ^ normY));
}

RotationMatrix RotationMatrix::fromY(const Vector3 &y)
{
    // Assuming z up
    Vector3 normY = y.safeNormalized();

    // If Y Parallel to Z then consider x fwd
    if (Math::isEqual(Math::abs(normY | Vector3::UP), 1.f, SLIGHTLY_SMALL_EPSILON))
    {
        Vector3 normZ = (Vector3::FWD ^ normY).safeNormalized();
        return RotationMatrix(Matrix3(normY ^ normZ, normY, normZ));
    }
    Vector3 normX = (normY ^ Vector3::UP).safeNormalized();
    return RotationMatrix(Matrix3(normX, normY, normX ^ normY));
}

RotationMatrix RotationMatrix::fromZ(const Vector3 &z)
{
    // Assuming x forward
    Vector3 normZ = z.safeNormalized();
    // If Z Parallel to X then consider y right
    if (Math::isEqual(Math::abs(normZ | Vector3::FWD), 1.f, SLIGHTLY_SMALL_EPSILON))
    {
        Vector3 normX = (Vector3::RIGHT ^ normZ).safeNormalized();
        return RotationMatrix(Matrix3(normX, normZ ^ normX, normZ));
    }
    Vector3 normY = (normZ ^ Vector3::FWD).safeNormalized();
    return RotationMatrix(Matrix3(normY ^ normZ, normY, normZ));
}

RotationMatrix RotationMatrix::fromXY(const Vector3 &x, const Vector3 &y)
{
    Vector3 normX = x.safeNormalized();
    Vector3 normY = y.rejectFrom(normX).safeNormalized();

    return RotationMatrix(Matrix3(normX, normY, normX ^ normY));
}

RotationMatrix RotationMatrix::fromYZ(const Vector3 &y, const Vector3 &z)
{
    Vector3 normZ = z.safeNormalized();
    Vector3 normY = y.rejectFrom(normZ).safeNormalized();

    return RotationMatrix(Matrix3(normY ^ normZ, normY, normZ));
}

RotationMatrix RotationMatrix::fromZX(const Vector3 &z, const Vector3 &x)
{
    Vector3 normX = x.safeNormalized();
    Vector3 normZ = z.rejectFrom(normX).safeNormalized();

    return RotationMatrix(Matrix3(normX, normZ ^ normX, normZ));
}

RotationMatrix RotationMatrix::fromXYZ(const Vector3 &x, const Vector3 &y, const Vector3 &z)
{
    RotationMatrix retVal(Matrix3(x.safeNormalized(), y, z));
    retVal.orthogonalize();
    return retVal;
}
