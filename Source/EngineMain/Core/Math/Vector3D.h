#pragma once

#include "CoreMathTypedefs.h"

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
    Vector3D normalized();
    Vector3D safeNormalize(float threshold = SMALL_EPSILON);
    float length();
    float sqrlength();

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector3D RIGHT;
    const static Vector3D FWD;
    const static Vector3D UP;
    const static Vector3D ZERO;
    const static Vector3D ONE;

    static float dot(const Vector3D& a, const Vector3D& b);
    static Vector3D cross(const Vector3D& a, const Vector3D& b);
};