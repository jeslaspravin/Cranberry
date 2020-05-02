#include "Transform3D.h"
#include "../Math/RotationMatrix.h"
#include "../Math/Vector4D.h"

Transform3D::Transform3D()
    : transformTranslation(0)
    , transformScale(1)
    , transformRotation(0)
    , transformMatrixCache(Matrix4::IDENTITY)
    , bCachedLatest(true)
{}

Transform3D::Transform3D(const Vector3D& translation, const Rotation& rotation, const Vector3D& scale)
    : transformTranslation(translation)
    , transformScale(scale)
    , transformRotation(rotation)
    , transformMatrixCache()
    , bCachedLatest(false)
{}

Transform3D::Transform3D(const Matrix4& transformMatrix)
    : transformTranslation(transformMatrix[3].x, transformMatrix[3].y, transformMatrix[3].z)
    , transformScale(Vector3D(transformMatrix[0]).length(),Vector3D(transformMatrix[1]).length()
        ,Vector3D(transformMatrix[2]).length())
    , transformRotation(RotationMatrix(transformMatrix / Matrix4(transformScale)).asRotation())
    , transformMatrixCache(transformMatrix)
    , bCachedLatest(true)
{}

Transform3D::Transform3D(const Transform3D& otherTransform)
    : transformTranslation(otherTransform.transformTranslation)
    , transformScale(otherTransform.transformScale)
    , transformRotation(otherTransform.transformRotation)
    , transformMatrixCache(otherTransform.transformMatrixCache)
    , bCachedLatest(otherTransform.bCachedLatest)
{}

Transform3D::Transform3D(Transform3D&& otherTransform)
    : transformTranslation(std::move(otherTransform.transformTranslation))
    , transformScale(std::move(otherTransform.transformScale))
    , transformRotation(std::move(otherTransform.transformRotation))
    , transformMatrixCache(std::move(otherTransform.transformMatrixCache))
    , bCachedLatest(std::move(otherTransform.bCachedLatest))
{}

void Transform3D::operator=(const Transform3D& otherTransform)
{
    transformTranslation = otherTransform.transformTranslation;
    transformScale = otherTransform.transformScale;
    transformRotation = otherTransform.transformRotation;
    transformMatrixCache = otherTransform.transformMatrixCache;
    bCachedLatest = otherTransform.bCachedLatest;
}

void Transform3D::operator=(Transform3D&& otherTransform)
{
    transformTranslation = std::move(otherTransform.transformTranslation);
    transformScale = std::move(otherTransform.transformScale);
    transformRotation = std::move(otherTransform.transformRotation);
    transformMatrixCache = std::move(otherTransform.transformMatrixCache);
    bCachedLatest = std::move(otherTransform.bCachedLatest);
}

Vector3D Transform3D::getTranslation() const
{
    return transformTranslation;
}

Rotation Transform3D::getRotation() const
{
    return transformRotation;
}

Vector3D Transform3D::getScale() const
{
    return transformScale;
}

void Transform3D::setTranslation(const Vector3D& newTranslation)
{
    transformTranslation = newTranslation;
    bCachedLatest = false;
}

void Transform3D::setRotation(const Rotation& newRotation)
{
    transformRotation = newRotation;
    bCachedLatest = false;
}

void Transform3D::setScale(const Vector3D& newScale)
{
    transformScale = newScale;
    bCachedLatest = false;
}

Matrix4 Transform3D::getTransformMatrix()
{
    if (!bCachedLatest)
    {
        RotationMatrix rotMatrix(transformRotation);
        transformMatrixCache[0] = Matrix4Col(rotMatrix.matrix()[0], 0.0f);
        transformMatrixCache[1] = Matrix4Col(rotMatrix.matrix()[1], 0.0f);
        transformMatrixCache[2] = Matrix4Col(rotMatrix.matrix()[2], 0.0f);

        transformMatrixCache |= Matrix4(transformScale);

        transformMatrixCache[3] = Matrix4Col(transformTranslation.x(), transformTranslation.y(), transformTranslation.z(), 1.0);

        bCachedLatest = true;
    }

    return transformMatrixCache;
}

Transform3D Transform3D::ZERO_TRANSFORM;
