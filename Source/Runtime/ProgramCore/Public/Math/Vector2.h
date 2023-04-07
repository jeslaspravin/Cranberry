/*!
 * \file Vector2.h
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

#include <glm/ext/vector_float2.hpp>

GLM_HEADER_INCLUDES_BEGIN

class Vector3;

class PROGRAMCORE_EXPORT Vector2
{
private:
    glm::vec2 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and
    // matrices
    friend class Matrix2;
    friend Vector2 operator/ (float n, const Vector2 &d);
    friend Vector2 operator- (float n, const Vector2 &d);

public:
    using value_type = glm::vec2::value_type;

public:
    Vector2();
    MAKE_TYPE_DEFAULT_COPY_MOVE(Vector2)
    explicit Vector2(const Matrix2Col &Vector2);
    Vector2(float x, float y);
    explicit Vector2(float allValue);
    Vector2(const Vector3 &other);

    float &x();
    float &y();
    float x() const;
    float y() const;
    float operator[] (uint32 index) const;
    float &operator[] (uint32 index);

public:
    bool operator== (const Vector2 &b) const;
    float operator| (const Vector2 &b) const;
    float operator^ (const Vector2 &b) const;
    // Component wise operations
    Vector2 operator* (const Vector2 &b) const;
    Vector2 &operator*= (const Vector2 &b);
    Vector2 operator/ (const Vector2 &b) const;
    Vector2 &operator/= (const Vector2 &b);
    Vector2 operator- (const Vector2 &b) const;
    Vector2 &operator-= (const Vector2 &b);
    Vector2 operator+ (const Vector2 &b) const;
    Vector2 &operator+= (const Vector2 &b);
    Vector2 operator* (float scalar) const;
    Vector2 &operator*= (float scalar);
    Vector2 operator/ (float scalar) const;
    Vector2 &operator/= (float scalar);
    Vector2 operator- (float scalar) const;
    Vector2 &operator-= (float scalar);
    Vector2 operator+ (float scalar) const;
    Vector2 &operator+= (float scalar);
    Vector2 operator- () const;

    bool isSame(const Vector2 &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    Vector2 safeInverse() const;
    Vector2 normalized() const;
    Vector2 safeNormalized(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector2 projectTo(const Vector2 &b) const;
    Vector2 rejectFrom(const Vector2 &b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector2 RIGHT;
    const static Vector2 FWD;
    const static Vector2 ZERO;
    const static Vector2 ONE;

    static float dot(const Vector2 &a, const Vector2 &b);
    static float cross(const Vector2 &a, const Vector2 &b);

    static Vector2 clamp(const Vector2 &value, const Vector2 &min, const Vector2 &max);
    static Vector2 min(const Vector2 &a, const Vector2 &b);
    static Vector2 max(const Vector2 &a, const Vector2 &b);
    static Vector2 abs(const Vector2 &value);
    static Vector2 floor(const Vector2 &value);
    static Vector2 ceil(const Vector2 &value);
    static Vector2 round(const Vector2 &value);
    static Vector2 mod(const Vector2 &a, const Vector2 &b);
    static Vector2 mod(const Vector2 &a, float b);
    static Vector2 modf(Vector2 &wholePart, const Vector2 &value);
};

FORCE_INLINE Vector2 operator/ (float n, const Vector2 &d) { return Vector2(n / d.value); }

FORCE_INLINE Vector2 operator- (float n, const Vector2 &d) { return Vector2(n - d.value); }

FORCE_INLINE Vector2 operator* (float n, const Vector2 &d) { return d * n; }

FORCE_INLINE Vector2 operator+ (float n, const Vector2 &d) { return d + n; }