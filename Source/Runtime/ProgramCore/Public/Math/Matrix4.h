/*!
 * \file Matrix4.h
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
#include "Serialization/ArchiveBase.h"
#include "ProgramCoreExports.h"

#include <glm/ext/matrix_float4x4.hpp>

class Vector3D;
class Vector4D;

class PROGRAMCORE_EXPORT Matrix4
{
private:
    glm::mat4 value;
    Matrix4(const glm::mat4 &matrix);

public:
    Matrix4();
    explicit Matrix4(float allValue);

    //  Matrix arrangement
    //  0-3B    4-7B    8-11B   12-15B
    //  r0(c0)  r0(c1)  r0(c2)  r0(c3)
    //  r1(c0)  r1(c1)  r1(c2)  r1(c3)
    //  r2(c0)  r2(c1)  r2(c2)  r2(c3)
    //  r3(c0)  r3(c1)  r3(c2)  r3(c3)
    Matrix4(
        const float &c1x, const float &c1y, const float &c1z, const float &c1w, const float &c2x, const float &c2y, const float &c2z,
        const float &c2w, const float &c3x, const float &c3y, const float &c3z, const float &c3w, const float &c4x, const float &c4y,
        const float &c4z, const float &c4w
    );
    Matrix4(const Vector3D &c1, const Vector3D &c2, const Vector3D &c3, const Vector3D &c4, const float &c4w = 1.0f);
    Matrix4(const Vector4D &c1, const Vector4D &c2, const Vector4D &c3, const Vector4D &c4);
    // Scaling matrix should always be applied before rotation or translation to preserve volume
    Matrix4(const Vector3D &scale);
    Matrix4(const Matrix4 &other);
    Matrix4(Matrix4 &&other);
    Matrix4 &operator=(const Matrix4 &other);
    Matrix4 &operator=(Matrix4 &&other);

    Matrix4Col &operator[](uint32 colIndex);
    Matrix4Col operator[](uint32 colIndex) const;

public:
    Vector4D operator*(const Vector4D &transformingVector) const;
    Vector3D operator*(const Vector3D &transformingVector) const;
    Matrix4 operator*(const Matrix4 &b) const;
    Matrix4 &operator*=(const Matrix4 &b);

    Matrix4 inverse() const;
    float determinant() const;
    Matrix4 transpose() const;

    // Component wise operations
    Matrix4 operator|(const Matrix4 &b) const;
    Matrix4 &operator|=(const Matrix4 &b);
    Matrix4 operator/(const Matrix4 &b) const;
    Matrix4 &operator/=(const Matrix4 &b);
    Matrix4 operator-(const Matrix4 &b) const;
    Matrix4 &operator-=(const Matrix4 &b);
    Matrix4 operator+(const Matrix4 &b) const;
    Matrix4 &operator+=(const Matrix4 &b);
    Matrix4 operator*(const float &scalar) const;
    Matrix4 &operator*=(const float &scalar);
    Matrix4 operator/(const float &scalar) const;
    Matrix4 &operator/=(const float &scalar);
    Matrix4 operator-(const float &scalar) const;
    Matrix4 &operator-=(const float &scalar);
    Matrix4 operator+(const float &scalar) const;
    Matrix4 &operator+=(const float &scalar);

public:
    static const Matrix4 IDENTITY;
};

template <ArchiveType ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Matrix4 &value)
{
    archive << value[0].x << value[1].x << value[2].x << value[3].x;
    archive << value[0].y << value[1].y << value[2].y << value[3].y;
    archive << value[0].z << value[1].z << value[2].z << value[3].z;
    archive << value[0].w << value[1].w << value[2].w << value[3].w;
    return archive;
}