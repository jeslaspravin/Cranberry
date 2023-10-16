/*!
 * \file RotationMatrix.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Types/CoreDefines.h"

class PROGRAMCORE_EXPORT RotationMatrix
{
private:
    Matrix3 rotationMatrix;
    FORCE_INLINE void fromRotation(const class Rotation &rotation);
    void verifyMatrix() const;

public:
    RotationMatrix();
    MAKE_TYPE_DEFAULT_COPY_MOVE(RotationMatrix)

    RotationMatrix(const Matrix3 &rotMatrix);
    RotationMatrix(const Matrix4 &rotMatrix);
    RotationMatrix(const class Rotation &rotation);

    Rotation asRotation() const;
    const Matrix3 &matrix() const;

    void orthogonalize();

    static RotationMatrix fromX(const Vector3 &x);
    static RotationMatrix fromY(const Vector3 &y);
    static RotationMatrix fromZ(const Vector3 &z);
    static RotationMatrix fromXY(const Vector3 &x, const Vector3 &y);
    static RotationMatrix fromYZ(const Vector3 &y, const Vector3 &z);
    static RotationMatrix fromZX(const Vector3 &z, const Vector3 &x);
    static RotationMatrix fromXYZ(const Vector3 &x, const Vector3 &y, const Vector3 &z);
};
