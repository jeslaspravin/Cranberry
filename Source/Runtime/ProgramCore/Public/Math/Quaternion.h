/*!
 * \file Quaternion.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Math/Rotation.h"
#include "Math/RotationMatrix.h"
#include "Math/Vector3.h"

class PROGRAMCORE_EXPORT Quat
{
public:
    float x = 0, y = 0, z = 0, w = 0;

private:
    void fromRotationImpl(Rotation rotation);
    void fromRotationMatImpl(const RotationMatrix &rotationMatrix);
    void fromAngleAxisImpl(float angle, Vector3 axis);

public:
    Quat() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(Quat)

    FORCE_INLINE Quat(float inX, float inY, float inZ, float inW)
        : x(inX)
        , y(inY)
        , z(inZ)
        , w(inW)
    {}
    FORCE_INLINE Quat(float angle, const Vector3 &axis) { fromAngleAxisImpl(angle, axis); }
    FORCE_INLINE Quat(const Rotation &rotation) { fromRotationImpl(rotation); }
    FORCE_INLINE Quat(const RotationMatrix &rotationMatrix) { fromRotationMatImpl(rotationMatrix); }

    float operator[] (uint32 index) const;
    float &operator[] (uint32 index);

    bool operator== (const Quat &b) const;
    float operator| (const Quat &b) const;
    // Similar to transforms & matrices. Right quaternion is applied inside the left quaternion's space
    Quat operator* (const Quat &b) const;
    Quat &operator*= (const Quat &b);
    // Component wise operations
    Quat operator- (const Quat &b) const;
    Quat &operator-= (const Quat &b);
    Quat operator+ (const Quat &b) const;
    Quat &operator+= (const Quat &b);
    Quat operator* (float scalar) const;
    Quat &operator*= (float scalar);
    Quat operator/ (float scalar) const;
    Quat &operator/= (float scalar);
    Quat operator- (float scalar) const;
    Quat &operator-= (float scalar);
    Quat operator+ (float scalar) const;
    Quat &operator+= (float scalar);
    Quat operator- () const;
    bool isSame(const Quat &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    bool isNan() const;
    Quat normalized() const;
    Quat safeNormalize(float threshold = SMALL_EPSILON) const;
    Quat inverse() const;
    float length() const;
    float sqrlength() const;

    static float dot(const Quat &a, const Quat &b);
    static Quat clamp(const Quat &value, const Quat &min, const Quat &max);
    static Quat floor(const Quat &value);
    static Quat ceil(const Quat &value);
    static Quat round(const Quat &value);

    // Other helpers
    const static Quat IDENTITY;

    Rotation toRotation() const;
    RotationMatrix toRotationMatrix() const;

    Vector3 rotateVector(const Vector3 &vector) const
    {
        Vector3 q{ x, y, z };
        return (w * w - q.sqrlength()) * vector + 2 * ((q | vector) * q + w * (q ^ vector));
    }

    FORCE_INLINE static Quat fromRotation(const Rotation &rotation)
    {
        Quat ret;
        ret.fromRotationImpl(rotation);
        return ret;
    }
    FORCE_INLINE static Quat fromAngleAxis(float angle, const Vector3 &axis)
    {
        Quat ret;
        ret.fromAngleAxisImpl(angle, axis);
        return ret;
    }
};

FORCE_INLINE Quat operator- (float n, const Quat &d) { return Quat(n - d.x, n - d.y, n - d.z, n - d.w); }

FORCE_INLINE Quat operator* (float n, const Quat &d) { return d * n; }

FORCE_INLINE Quat operator+ (float n, const Quat &d) { return d + n; }

//////////////////////////////////////////////////////////////////////////
/// Implementations
//////////////////////////////////////////////////////////////////////////

inline float Quat::operator[] (uint32 index) const { return (&this->x)[index]; }
inline float &Quat::operator[] (uint32 index) { return (&this->x)[index]; }

inline float Quat::operator| (const Quat &b) const { return (x * b.x) + (y * b.y) + (z * b.z) + (w * b.w); }

inline Quat Quat::operator* (const Quat &b) const
{
    Quat ret(*this);
    return ret *= b;
}
inline Quat &Quat::operator*= (const Quat &b)
{
    Quat a(*this);
    this->x = a.x * b.w + a.w * b.x + a.y * b.z - a.z * b.y;
    this->y = a.y * b.w + a.w * b.y - a.x * b.z + a.z * b.x;
    this->z = a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x;
    this->w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;

    *this = this->safeNormalize();
    return (*this);
}

inline Quat Quat::operator* (float scalar) const { return Quat(x * scalar, y * scalar, z * scalar, w * scalar); }
inline Quat &Quat::operator*= (float scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

inline Quat Quat::operator/ (float scalar) const { return Quat(x / scalar, y / scalar, z / scalar, w / scalar); }
inline Quat &Quat::operator/= (float scalar)
{
    x /= scalar;
    y /= scalar;
    z /= scalar;
    w /= scalar;
    return *this;
}

inline Quat Quat::operator- (const Quat &b) const { return Quat(x - b.x, y - b.y, z - b.z, w - b.w); }
inline Quat &Quat::operator-= (const Quat &b)
{
    x -= b.x;
    y -= b.y;
    z -= b.z;
    w -= b.w;
    return *this;
}

inline Quat Quat::operator- (float scalar) const { return Quat(x - scalar, y - scalar, z - scalar, w - scalar); }
inline Quat &Quat::operator-= (float scalar)
{
    x -= scalar;
    y -= scalar;
    z -= scalar;
    w -= scalar;
    return *this;
}

inline Quat Quat::operator- () const { return Quat(-x, -y, -z, -w); }

inline Quat Quat::operator+ (const Quat &b) const { return Quat(x + b.x, y + b.y, z + b.z, w + b.w); }
inline Quat &Quat::operator+= (const Quat &b)
{
    x += b.x;
    y += b.y;
    z += b.z;
    w += b.w;
    return *this;
}

inline Quat Quat::operator+ (float scalar) const { return Quat(x + scalar, y + scalar, z + scalar, w + scalar); }
inline Quat &Quat::operator+= (float scalar)
{
    x += scalar;
    y += scalar;
    z += scalar;
    w += scalar;
    return *this;
}

inline float Quat::dot(const Quat &a, const Quat &b) { return a | b; }