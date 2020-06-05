#pragma once

#include "CoreMathTypedefs.h"
#include "../Types/CoreDefines.h"

#include <glm/ext/vector_float3.hpp>

class Vector3D
{
private:
    glm::vec3 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and matrices
    friend class Matrix3;
public:
    Vector3D();
    explicit Vector3D(const Matrix3Col& vector3d);
    Vector3D(const float& x, const float& y, const float& z);
    Vector3D(const float& x, const float& y);
    explicit Vector3D(const float& allValue);
    Vector3D(const Vector3D& other);
    Vector3D(Vector3D&& other);
    void operator=(const Vector3D& other);
    void operator=(Vector3D&& other);

    float& x();
    float& y();
    float& z();
    float x() const;
    float y() const;
    float z() const;
    float operator[](uint32 index) const;
    float& operator[](uint32 index);

public:
    bool operator==(const Vector3D& b) const;
    float operator|(const Vector3D& b) const;
    Vector3D operator^(const Vector3D& b) const;
    // Component wise operations
    Vector3D operator*(const Vector3D& b) const;
    void operator*=(const Vector3D& b);
    Vector3D operator/(const Vector3D& b) const;
    void operator/=(const Vector3D& b);
    Vector3D operator-(const Vector3D& b) const;
    void operator-=(const Vector3D& b);
    Vector3D operator+(const Vector3D& b) const;
    void operator+=(const Vector3D& b);
    Vector3D operator*(const float& scalar) const;
    void operator*=(const float& scalar);
    Vector3D operator/(const float& scalar) const;
    void operator/=(const float& scalar);
    Vector3D operator-(const float& scalar) const;
    void operator-=(const float& scalar);
    Vector3D operator+(const float& scalar) const;
    void operator+=(const float& scalar);
    bool isSame(const Vector3D& b, float epsilon = SMALL_EPSILON) const;
    Vector3D normalized() const;
    Vector3D safeNormalize(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector3D projectTo(const Vector3D& b) const;
    Vector3D rejectFrom(const Vector3D& b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector3D RIGHT;
    const static Vector3D FWD;
    const static Vector3D UP;
    const static Vector3D ZERO;
    const static Vector3D ONE;

    static FORCE_INLINE float dot(const Vector3D& a, const Vector3D& b);
    static FORCE_INLINE Vector3D cross(const Vector3D& a, const Vector3D& b);

    static FORCE_INLINE Vector3D clamp(const Vector3D& value, const Vector3D& min, const Vector3D& max);
    static FORCE_INLINE Vector3D min(const Vector3D& a, const Vector3D& b);
    static FORCE_INLINE Vector3D max(const Vector3D& a, const Vector3D& b);
    static FORCE_INLINE Vector3D abs(const Vector3D& value);
    static FORCE_INLINE Vector3D floor(const Vector3D& value);
    static FORCE_INLINE Vector3D ceil(const Vector3D& value);
};