/*!
 * \file Transform3D.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Serialization/ArchiveTypes.h"
#include "Math/Matrix4.h"
#include "Math/Rotation.h"
#include "Math/Vector3.h"

class PROGRAMCORE_EXPORT Transform3D
{
private:
    Vector3 transformTranslation;
    Vector3 transformScale;
    Rotation transformRotation;

    template <ArchiveTypeName ArchiveType>
    friend ArchiveType &operator<< (ArchiveType &archive, Transform3D &value);

public:
    static Transform3D ZERO_TRANSFORM;

public:
    Transform3D();
    Transform3D(const Vector3 &translation, const Rotation &rotation, const Vector3 &scale);
    Transform3D(const Rotation &rotation);
    // Matrix should be proper orthogonal matrix, Some non uniform scaled matrices won't create proper
    // transform, In those cases use raw Matrix
    Transform3D(const Matrix4 &transformMatrix);
    Transform3D(const Transform3D &otherTransform);
    Transform3D(Transform3D &&otherTransform);
    Transform3D &operator= (const Transform3D &otherTransform);
    Transform3D &operator= (Transform3D &&otherTransform);
    Transform3D &operator= (const Matrix4 &transformMatrix);
    bool isSame(const Transform3D &b, float epsilon = SMALL_EPSILON) const;

    const Vector3 &getTranslation() const;
    const Rotation &getRotation() const;
    const Vector3 &getScale() const;
    Vector3 &getTranslation();
    Rotation &getRotation();
    Vector3 &getScale();
    void setTranslation(const Vector3 &newTranslation);
    void setRotation(const Rotation &newRotation);
    void setScale(const Vector3 &newScale);

    Matrix4 getTransformMatrix() const;
    Vector3 transformNormal(const Vector3 &normal) const;
    Vector3 invTransformNormal(const Vector3 &normal) const;
    Vector3 transformPoint(const Vector3 &point) const;
    Vector3 invTransformPoint(const Vector3 &point) const;
    Transform3D transform(const Transform3D &other) const;
    Transform3D invTransform(const Transform3D &other) const;
    Transform3D inverseNonUniformScaled() const;

    // Below functions works only for uniform scale for non uniform scale use corresponding
    // *NonUniformScaled
    Transform3D inverse() const;

private:
    Matrix4 normalTransformMatrix() const;

    Vector3 invScaleSafe() const;
    Vector3 invTranslation() const;

    Matrix4 inverseNonUniformScaledMatrix() const;
};