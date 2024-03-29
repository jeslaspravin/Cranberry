/*!
 * \file Quaternion.cpp
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Quaternion.h"
#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"

void Quat::fromRotationImpl(Rotation rotation)
{
    rotation /= 2;
    Rotation s = Math::sin(rotation);
    Rotation c = Math::cos(rotation);

    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/index.htm
    // Heading is Roll
    // Attitude is Pitch
    // Bank is Yaw
    // Not suitable for engine's rotation order roll-pitch-yaw
    // x = s.roll() * s.pitch() * c.yaw() + c.roll() * c.pitch() * s.yaw();
    // y = s.roll() * c.pitch() * c.yaw() + c.roll() * s.pitch() * s.yaw();
    // z = c.roll() * s.pitch() * c.yaw() - s.roll() * c.pitch() * s.yaw();
    // w = c.roll() * c.pitch() * c.yaw() - s.roll() * s.pitch() * s.yaw();

    // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Euler_angles_to_quaternion_conversion
    x = s.roll() * c.pitch() * c.yaw() - c.roll() * s.pitch() * s.yaw();
    y = c.roll() * s.pitch() * c.yaw() + s.roll() * c.pitch() * s.yaw();
    z = c.roll() * c.pitch() * s.yaw() - s.roll() * s.pitch() * c.yaw();
    w = c.roll() * c.pitch() * c.yaw() + s.roll() * s.pitch() * s.yaw();
}

Rotation Quat::toRotation() const
{
    float qxx(x * x);
    float qyy(y * y);
    float qzz(z * z);
    float qxz(x * z);
    float qxy(x * y);
    float qyz(y * z);
    float qwx(w * x);
    float qwy(w * y);
    float qwz(w * z);

    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/index.htm
    // float roll = Math::atan(2 * (qwy - qxz), 1.0f - 2 * (qyy + qzz));
    // float pitch = Math::asin(2 * (qxy + qwz));
    // float yaw = Math::atan(2 * (qwx - qyz), 1.0f - 2 * (qxx + qzz));

    // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Quaternion_to_Euler_angles_conversion
    float roll = Math::atan(2 * (qwx + qyz), 1.0f - 2 * (qxx + qyy));
    // There is chance when using arc-sin that the input to arc-sin might be outside valid range.
    // That is because of square length precision loss while normalizing
    float pitch = Math::asin(Math::clamp(2 * (qwy - qxz), -1.f, 1.f));
    float yaw = Math::atan(2 * (qwz + qxy), 1.0f - 2 * (qyy + qzz));

    return Math::rad2Deg(Rotation(roll, pitch, yaw));
}

// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
// Is same as in wikipedia
// https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion
// Matrix transposed from row major to column major
void Quat::fromRotationMatImpl(const RotationMatrix &rotationMatrix)
{
    const Matrix3 &rotMat = rotationMatrix.matrix();
    const float trace = rotMat[0][0] + rotMat[1][1] + rotMat[2][2];

    if (trace > 0.0f)
    {
        float s = Math::sqrt(trace + 1.0f);
        w = 0.5f * s;

        float t = 0.5f / s;
        x = (rotMat[1][2] - rotMat[2][1]) * t;
        y = (rotMat[2][0] - rotMat[0][2]) * t;
        z = (rotMat[0][1] - rotMat[1][0]) * t;
    }
    else
    {
        // Find largest in trace
        int32 i = 0;
        if (rotMat[1][1] > rotMat[0][0])
        {
            i = 1;
        }
        if (rotMat[2][2] > rotMat[i][i])
        {
            i = 2;
        }

        const static int32 NEXTS[] = { 1, 2, 0 };
        int32 j = NEXTS[i];
        int32 k = NEXTS[j];

        float s = Math::sqrt(1.0f + rotMat[i][i] - rotMat[j][j] - rotMat[k][k]);
        this->operator[] (i) = 0.5f * s;

        // S not possible to be 0.0
        float t = 0.5f / s;
        this->operator[] (j) = (rotMat[i][j] + rotMat[j][i]) * t;
        this->operator[] (k) = (rotMat[i][k] + rotMat[k][i]) * t;
        w = (rotMat[j][k] - rotMat[k][j]) * t;
    }
}

// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
// Is same as in wikipedia
// https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion
// Matrix transposed from row major to column major
RotationMatrix Quat::toRotationMatrix() const
{
    float qxx(x * x);
    float qyy(y * y);
    float qzz(z * z);
    float qxz(x * z);
    float qxy(x * y);
    float qyz(y * z);
    float qwx(w * x);
    float qwy(w * y);
    float qwz(w * z);

    Matrix3 rotMat(
        /* Column 1 */ 0.5f - qyy - qzz, qxy + qwz,
        qxz - qwy
        /* Column 2 */,
        qxy - qwz, 0.5f - qxx - qzz,
        qyz + qwx
        /* Column 3 */,
        qxz + qwy, qyz - qwx, 0.5f - qxx - qyy
    );
    rotMat *= 2;

    return RotationMatrix(rotMat);
}

void Quat::fromAngleAxisImpl(float angle, Vector3 axis)
{
    const float hAngleRad = Math::deg2Rad(angle) * 0.5f;
    const float hAngleSin = Math::sin(hAngleRad);

    float sqrLen = axis.sqrlength();
    alertAlwaysf(sqrLen >= SMALL_EPSILON, "Invalid axis square length {}", sqrLen);
    if (sqrLen >= SMALL_EPSILON && !Math::isEqual(1.0f, sqrLen, SMALL_EPSILON)) [[unlikely]]
    {
        axis = axis.normalized();
    }
    axis *= hAngleSin;
    x = axis.x();
    y = axis.y();
    z = axis.z();
    w = Math::cos(hAngleRad);
}

bool Quat::operator== (const Quat &b) const
{
    return Math::isEqual(x, b.x) && Math::isEqual(y, b.y) && Math::isEqual(z, b.z) && Math::isEqual(w, b.w);
}

bool Quat::isSame(const Quat &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(x, b.x, epsilon) && Math::isEqual(y, b.y, epsilon) && Math::isEqual(z, b.z, epsilon) && Math::isEqual(w, b.w, epsilon);
}

bool Quat::isFinite() const { return Math::isFinite(x) && Math::isFinite(y) && Math::isFinite(z) && Math::isFinite(w); }
bool Quat::isNan() const { return Math::isNan(x) || Math::isNan(y) || Math::isNan(z) || Math::isNan(w); }

Quat Quat::normalized() const { return (*this) * Math::invSqrt(sqrlength()); }

Quat Quat::safeNormalize(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < threshold)
    {
        return IDENTITY;
    }
    // https://stackoverflow.com/a/12934750 If normalizing often allows integration based normalization
    if (Math::isEqual(sqrLen, 1.0f, 2.00050249077e-06f))
    {
        return Quat((*this) * float(2.0 / (1.0 + sqrLen)));
    }
    return Quat((*this) * Math::invSqrt(sqrLen));
}

Quat Quat::inverse() const
{
    Quat retVal(*this);
    if (!Math::isEqual(sqrlength(), 1.0f, SLIGHTLY_SMALL_EPSILON))
    {
        retVal = safeNormalize();
    }
    retVal.x = -retVal.x;
    retVal.y = -retVal.y;
    retVal.z = -retVal.z;
    return retVal;
}

float Quat::length() const { return Math::sqrt(sqrlength()); }

float Quat::sqrlength() const { return (x * x) + (y * y) + (z * z) + (w * w); }

Quat Quat::clamp(const Quat &value, const Quat &min, const Quat &max)
{
    return Quat(
        Math::clamp(value.x, min.x, max.x), Math::clamp(value.y, min.y, max.y), Math::clamp(value.z, min.z, max.z),
        Math::clamp(value.w, min.w, max.w)
    );
}

Quat Quat::floor(const Quat &value) { return Quat(Math::floor(value.x), Math::floor(value.y), Math::floor(value.z), Math::floor(value.w)); }

Quat Quat::ceil(const Quat &value) { return Quat(Math::ceil(value.x), Math::ceil(value.y), Math::ceil(value.z), Math::ceil(value.w)); }

Quat Quat::round(const Quat &value) { return Quat(Math::round(value.x), Math::round(value.y), Math::round(value.z), Math::round(value.w)); }

const Quat Quat::IDENTITY(0, 0, 0, 1);
