#pragma once
#include "../Math/Vector3D.h"
#include "../Math/Rotation.h"
#include "../Math/Matrix4.h"

class Transform3D
{
private:
    Vector3D transformTranslation;
    Vector3D transformScale;
    Rotation transformRotation;

    Matrix4 transformMatrixCache;
    bool bCachedLatest;
public:
    Transform3D();
    Transform3D(const Vector3D& translation, const Rotation& rotation, const Vector3D& scale);
    // Matrix should be proper orthogonal matrix
    Transform3D(const Matrix4& transformMatrix);
    Transform3D(const Transform3D& otherTransform);
    Transform3D(Transform3D&& otherTransform);
    void operator=(const Transform3D& otherTransform);
    void operator=(Transform3D&& otherTransform);

    Vector3D getTranslation() const;
    Rotation getRotation() const;
    Vector3D getScale() const;
    void setTranslation(const Vector3D& newTranslation);
    void setRotation(const Rotation& newRotation);
    void setScale(const Vector3D& newScale);

    Matrix4 getTransformMatrix();

public:
    static Transform3D ZERO_TRANSFORM;
};