/*!
 * \file Quaternion.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Math/Rotation.h"
#include "Math/RotationMatrix.h"
#include "Math/Vector3D.h"

class PROGRAMCORE_EXPORT Quat
{
public:
    float x = 0, y = 0, z = 0, w = 0;

private:
    void fromRotationImpl(Rotation rotation);
    void fromRotationMatImpl(const RotationMatrix &rotationMatrix);
    void fromAngleAxisImpl(float angle, Vector3D axis);

public:
    Quat() = default;
    Quat(const Quat &) = default;
    Quat(Quat &&) = default;
    FORCE_INLINE Quat(float inX, float inY, float inZ, float inW)
        : x(inX)
        , y(inY)
        , z(inZ)
        , w(inW)
    {}
    FORCE_INLINE Quat(float angle, const Vector3D &axis) { fromAngleAxisImpl(angle, axis); }
    FORCE_INLINE Quat(const Rotation &rotation) { fromRotationImpl(rotation); }
    FORCE_INLINE Quat(const RotationMatrix &rotationMatrix) { fromRotationMatImpl(rotationMatrix); }

    Quat &operator=(const Quat &other) = default;
    Quat &operator=(Quat &&other) = default;

    float operator[](uint32 index) const;
    float &operator[](uint32 index);

    bool operator==(const Quat &b) const;
    float operator|(const Quat &b) const;
    Quat operator*(const Quat &b) const;
    Quat &operator*=(const Quat &b);
    // Component wise operations
    Quat operator-(const Quat &b) const;
    Quat &operator-=(const Quat &b);
    Quat operator+(const Quat &b) const;
    Quat &operator+=(const Quat &b);
    Quat operator*(const float &scalar) const;
    Quat &operator*=(const float &scalar);
    Quat operator/(const float &scalar) const;
    Quat &operator/=(const float &scalar);
    Quat operator-(const float &scalar) const;
    Quat &operator-=(const float &scalar);
    Quat operator+(const float &scalar) const;
    Quat &operator+=(const float &scalar);
    Quat operator-() const;
    bool isSame(const Quat &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    Quat normalized() const;
    Quat safeNormalize(float threshold = SMALL_EPSILON) const;
    Quat inverse() const;
    float length() const;
    float sqrlength() const;

    static float dot(const Vector3D &a, const Vector3D &b);
    static Quat clamp(const Quat &value, const Quat &min, const Quat &max);
    static Quat floor(const Quat &value);
    static Quat ceil(const Quat &value);
    static Quat round(const Quat &value);

    // Other helpers
    const static Quat IDENTITY;

    Rotation toRotation() const;
    RotationMatrix toRotationMatrix() const;
    Vector3D rotateVector(const Vector3D &vector) const;

    static float dot(const Quat &a, const Quat &b);
    FORCE_INLINE static Quat fromRotation(const Rotation &rotation)
    {
        Quat ret;
        ret.fromRotationImpl(rotation);
        return ret;
    }
    FORCE_INLINE static Quat fromAngleAxis(float angle, const Vector3D &axis)
    {
        Quat ret;
        ret.fromAngleAxisImpl(angle, axis);
        return ret;
    }
};

FORCE_INLINE Quat operator-(float n, const Quat &d) { return Quat(n - d.x, n - d.y, n - d.z, n - d.w); }

FORCE_INLINE Quat operator*(float n, const Quat &d) { return d * n; }

FORCE_INLINE Quat operator+(float n, const Quat &d) { return d + n; }
