/*!
 * \file Vector3.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypedefs.h"
#include "ProgramCoreExports.h"
#include "Types/CoreDefines.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/ext/vector_float3.hpp>

GLM_HEADER_INCLUDES_BEGIN

class Vector4;
class Vector2;

class PROGRAMCORE_EXPORT Vector3
{
private:
    glm::vec3 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and
    // matrices
    friend class Matrix3;
    friend Vector3 operator/ (float n, const Vector3 &d);
    friend Vector3 operator- (float n, const Vector3 &d);

public:
    using value_type = glm::vec3::value_type;

public:
    Vector3();
    MAKE_TYPE_DEFAULT_COPY_MOVE(Vector3)
    explicit Vector3(const Matrix3Col &Vector3);
    Vector3(float x, float y, float z);
    Vector3(float x, float y);
    explicit Vector3(float allValue);
    Vector3(const Vector2 &xy, float z);
    // Construct from v4d x,y,z
    Vector3(const Vector4 &other);

    float &x();
    float &y();
    float &z();
    float x() const;
    float y() const;
    float z() const;
    float operator[] (uint32 index) const;
    float &operator[] (uint32 index);

public:
    bool operator== (const Vector3 &b) const;
    float operator| (const Vector3 &b) const;
    Vector3 operator^ (const Vector3 &b) const;
    // Component wise operations
    Vector3 operator* (const Vector3 &b) const;
    Vector3 &operator*= (const Vector3 &b);
    Vector3 operator/ (const Vector3 &b) const;
    Vector3 &operator/= (const Vector3 &b);
    Vector3 operator- (const Vector3 &b) const;
    Vector3 &operator-= (const Vector3 &b);
    Vector3 operator+ (const Vector3 &b) const;
    Vector3 &operator+= (const Vector3 &b);
    Vector3 operator* (float scalar) const;
    Vector3 &operator*= (float scalar);
    Vector3 operator/ (float scalar) const;
    Vector3 &operator/= (float scalar);
    Vector3 operator- (float scalar) const;
    Vector3 &operator-= (float scalar);
    Vector3 operator+ (float scalar) const;
    Vector3 &operator+= (float scalar);
    Vector3 operator- () const;
    bool isSame(const Vector3 &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    Vector3 safeInverse() const;
    Vector3 normalized() const;
    Vector3 safeNormalized(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector3 projectTo(const Vector3 &b) const;
    Vector3 rejectFrom(const Vector3 &b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector3 RIGHT;
    const static Vector3 FWD;
    const static Vector3 UP;
    const static Vector3 ZERO;
    const static Vector3 ONE;

    static float dot(const Vector3 &a, const Vector3 &b);
    static Vector3 cross(const Vector3 &a, const Vector3 &b);

    static Vector3 clamp(const Vector3 &value, const Vector3 &min, const Vector3 &max);
    static Vector3 min(const Vector3 &a, const Vector3 &b);
    static Vector3 max(const Vector3 &a, const Vector3 &b);
    static Vector3 abs(const Vector3 &value);
    static Vector3 floor(const Vector3 &value);
    static Vector3 ceil(const Vector3 &value);
    static Vector3 round(const Vector3 &value);
    static Vector3 mod(const Vector3 &a, const Vector3 &b);
    static Vector3 mod(const Vector3 &a, float b);
    static Vector3 modf(Vector3 &wholePart, const Vector3 &value);
};

FORCE_INLINE Vector3 operator/ (float n, const Vector3 &d) { return Vector3(n / d.value); }

FORCE_INLINE Vector3 operator- (float n, const Vector3 &d) { return Vector3(n - d.value); }

FORCE_INLINE Vector3 operator* (float n, const Vector3 &d) { return d * n; }

FORCE_INLINE Vector3 operator+ (float n, const Vector3 &d) { return d + n; }