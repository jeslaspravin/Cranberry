/*!
 * \file Plane.h
 *
 * \author Jeslas
 * \date June 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

class Plane
{
private:
    Vector4 value;

public:
    using value_type = Vector4::value_type;

public:
    Plane() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(Plane)

    Plane(float nX, float nY, float nZ, float d);
    explicit Plane(Vector4 vec4);
    explicit Plane(const Matrix4Col &vec4);
    Plane(Vector3 n, float d);

    float &x();
    float &y();
    float &z();
    float &w();
    float x() const;
    float y() const;
    float z() const;
    float w() const;
    float operator[] (uint32 index) const;
    float &operator[] (uint32 index);

public:
    bool operator== (const Plane &b) const;
    float operator| (const Vector4 &b) const;
    float operator| (const Vector3 &b) const;

    Plane operator* (float dOffset) const;
    Plane &operator*= (float dOffset);
    Plane operator/ (float dOffset) const;
    Plane &operator/= (float dOffset);
    Plane operator- (float dOffset) const;
    Plane &operator-= (float dOffset);
    Plane operator+ (float dOffset) const;
    Plane &operator+= (float dOffset);

    Plane operator- () const;

    bool isSame(const Plane &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    Plane normalized() const;
    Plane safeNormalized(float threshold = SMALL_EPSILON) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Plane XZ;
    const static Plane YZ;
    const static Plane XY;

    static float dot(const Plane &plane, const Vector3 &b);
};

//////////////////////////////////////////////////////////////////////////
/// Implementations
//////////////////////////////////////////////////////////////////////////

inline Plane::Plane(Vector4 val)
    : value(val)
{}
inline Plane::Plane(const Matrix4Col &vec4)
    : value(vec4)
{}
inline Plane::Plane(float nX, float nY, float nZ, float d)
    : value(nX, nY, nZ, d)
{}
inline Plane::Plane(Vector3 n, float d)
    : value(n, d)
{}

inline float &Plane::x() { return value.x(); }
inline float Plane::x() const { return value.x(); }

inline float &Plane::y() { return value.y(); }
inline float Plane::y() const { return value.y(); }

inline float &Plane::z() { return value.z(); }
inline float Plane::z() const { return value.z(); }

inline float &Plane::w() { return value.w(); }
inline float Plane::w() const { return value.w(); }

inline float Plane::operator[] (uint32 index) const { return value[index]; }
inline float &Plane::operator[] (uint32 index) { return value[index]; }

inline bool Plane::operator== (const Plane &b) const { return isSame(b); }

inline float Plane::operator| (const Vector4 &b) const { return value | b; }
inline float Plane::operator| (const Vector3 &b) const { return value | Vector4(b, 1); }

inline Plane Plane::operator* (float dOffset) const
{
    Plane temp{ value };
    temp.w() *= dOffset;
    return temp;
}
inline Plane &Plane::operator*= (float dOffset)
{
    value.w() *= dOffset;
    return *this;
}

inline Plane Plane::operator/ (float dOffset) const
{
    Plane temp{ value };
    temp.w() /= dOffset;
    return temp;
}
inline Plane &Plane::operator/= (float dOffset)
{
    value.w() /= dOffset;
    return *this;
}

inline Plane Plane::operator- (float dOffset) const
{
    Plane temp{ value };
    temp.w() -= dOffset;
    return temp;
}
inline Plane &Plane::operator-= (float dOffset)
{
    value.w() -= dOffset;
    return *this;
}

inline Plane Plane::operator+ (float dOffset) const
{
    Plane temp{ value };
    temp.w() *= dOffset;
    return temp;
}
inline Plane &Plane::operator+= (float dOffset)
{
    value.w() += dOffset;
    return *this;
}

inline Plane Plane::operator- () const
{
    Plane temp{ value };
    temp.w() = -temp.w();
    return temp;
}

inline Plane Plane::normalized() const { return Plane(value * (1 / value.length3())); }

inline float Plane::dot(const Plane &plane, const Vector3 &b) { return plane | b; }