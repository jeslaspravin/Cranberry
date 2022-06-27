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
#include "Math/Vector3D.h"

class PROGRAMCORE_EXPORT Transform3D
{
private:
    Vector3D transformTranslation;
    Vector3D transformScale;
    Rotation transformRotation;

    Matrix4 transformMatrixCache;
    bool bCachedLatest;
    
    template <ArchiveTypeName ArchiveType>
    friend ArchiveType &operator<<(ArchiveType &archive, Transform3D &value);
private:
    Matrix4 normalTransformMatrix() const;
    Matrix4 getTransformMatrix() const;

    Vector3D invScaleSafe() const;
    Vector3D invTranslation() const;

public:
    Transform3D();
    Transform3D(const Vector3D &translation, const Rotation &rotation, const Vector3D &scale);
    Transform3D(const Rotation &rotation);
    // Matrix should be proper orthogonal matrix, Some non uniform scaled matrices won't create proper
    // transform, In those cases use raw Matrix
    Transform3D(const Matrix4 &transformMatrix);
    Transform3D(const Transform3D &otherTransform);
    Transform3D(Transform3D &&otherTransform);
    void operator=(const Transform3D &otherTransform);
    void operator=(Transform3D &&otherTransform);
    void operator=(const Matrix4 &transformMatrix);

    const Vector3D &getTranslation() const;
    const Rotation &getRotation() const;
    const Vector3D &getScale() const;
    Vector3D &getTranslation();
    Rotation &getRotation();
    Vector3D &getScale();
    void setTranslation(const Vector3D &newTranslation);
    void setRotation(const Rotation &newRotation);
    void setScale(const Vector3D &newScale);

    const Matrix4 &getTransformMatrix();
    Vector3D transformNormal(const Vector3D &normal) const;
    Vector3D invTransformNormal(const Vector3D &normal) const;
    Vector3D transformPoint(const Vector3D &point) const;
    Vector3D invTransformPoint(const Vector3D &point) const;
    Transform3D transform(const Transform3D &other);
    Transform3D invTransform(const Transform3D &other);
    Transform3D inverseNonUniformScaled() const;

    // Below functions works only for uniform scale for non uniform scale use corresponding
    // *NonUniformScaled
    Transform3D inverse() const;

public:
    static Transform3D ZERO_TRANSFORM;
};