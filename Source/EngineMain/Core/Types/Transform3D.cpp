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

Transform3D::Transform3D(const Rotation& rotation)
    : transformTranslation(Vector3D::ZERO)
    , transformScale(Vector3D::ONE)
    , transformRotation(rotation)
    , transformMatrixCache()
    , bCachedLatest(false)
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

void Transform3D::operator=(const Matrix4& transformMatrix)
{
    transformTranslation = Vector3D(transformMatrix[3].x, transformMatrix[3].y, transformMatrix[3].z);
    transformScale = Vector3D(Vector3D(transformMatrix[0]).length(), Vector3D(transformMatrix[1]).length()
        , Vector3D(transformMatrix[2]).length());
    transformRotation = Rotation(RotationMatrix(transformMatrix / Matrix4(transformScale)).asRotation());
    transformMatrixCache = transformMatrix;
    bCachedLatest = true;
}

const Vector3D& Transform3D::getTranslation() const
{
    return transformTranslation;
}

Vector3D& Transform3D::getTranslation()
{
    bCachedLatest = false;
    return transformTranslation;
}

const Rotation& Transform3D::getRotation() const
{
    return transformRotation;
}

Rotation& Transform3D::getRotation()
{
    bCachedLatest = false;
    return transformRotation;
}

const Vector3D& Transform3D::getScale() const
{
    return transformScale;
}

Vector3D& Transform3D::getScale()
{
    bCachedLatest = false;
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

Matrix4 Transform3D::normalTransformMatrix() const
{
    Matrix4 normTranform;
    RotationMatrix rotMatrix(transformRotation);
    normTranform[0] = Matrix4Col(rotMatrix.matrix()[0], 0.0f);
    normTranform[1] = Matrix4Col(rotMatrix.matrix()[1], 0.0f);
    normTranform[2] = Matrix4Col(rotMatrix.matrix()[2], 0.0f);
    // Inversing scale alone
    normTranform *= Matrix4(Vector3D::ONE / transformScale);
    return normTranform;
}

const Matrix4& Transform3D::getTransformMatrix()
{
    if (!bCachedLatest)
    {
        RotationMatrix rotMatrix(transformRotation);
        transformMatrixCache[0] = Matrix4Col(rotMatrix.matrix()[0], 0.0f);
        transformMatrixCache[1] = Matrix4Col(rotMatrix.matrix()[1], 0.0f);
        transformMatrixCache[2] = Matrix4Col(rotMatrix.matrix()[2], 0.0f);

        transformMatrixCache *= Matrix4(transformScale);

        transformMatrixCache[3] = Matrix4Col(transformTranslation.x(), transformTranslation.y(), transformTranslation.z(), 1.0);

        bCachedLatest = true;
    }

    return transformMatrixCache;
}

Matrix4 Transform3D::getTransformMatrix() const
{
    if (bCachedLatest)
    {
        return transformMatrixCache;
    }
    Matrix4 transformMatrix;
    RotationMatrix rotMatrix(transformRotation);
    transformMatrix[0] = Matrix4Col(rotMatrix.matrix()[0], 0.0f);
    transformMatrix[1] = Matrix4Col(rotMatrix.matrix()[1], 0.0f);
    transformMatrix[2] = Matrix4Col(rotMatrix.matrix()[2], 0.0f);

    transformMatrix *= Matrix4(transformScale);

    transformMatrix[3] = Matrix4Col(transformTranslation.x(), transformTranslation.y(), transformTranslation.z(), 1.0);

    return transformMatrix;
}

Vector3D Transform3D::transformNormal(const Vector3D& normal) const
{
    Vector4D transformed = normalTransformMatrix() * Vector4D(normal.x(), normal.y(), normal.z(), 1);
    return Vector3D(transformed.x(), transformed.y(), transformed.z());
}

Vector3D Transform3D::invTransformNormal(const Vector3D& normal) const
{
    Vector4D transformed = (normalTransformMatrix().inverse()) * Vector4D(normal.x(), normal.y(), normal.z(), 1);
    return Vector3D(transformed.x(), transformed.y(), transformed.z());
}

Vector3D Transform3D::transformPoint(const Vector3D& point)
{
    Vector4D transformed = getTransformMatrix() * Vector4D(point.x(), point.y(), point.z(), 1);
    return Vector3D(transformed.x(), transformed.y(), transformed.z());
}

Vector3D Transform3D::invTransformPoint(const Vector3D& point)
{
    Vector4D transformed = (getTransformMatrix().inverse()) * Vector4D(point.x(), point.y(), point.z(), 1);
    return Vector3D(transformed.x(), transformed.y(), transformed.z());
}

Transform3D Transform3D::transform(const Transform3D& other)
{
    return Transform3D{ getTransformMatrix() * other.getTransformMatrix() };
}

Transform3D Transform3D::invTransform(const Transform3D& other)
{
    return Transform3D{ (getTransformMatrix().inverse()) * other.getTransformMatrix() };
}

Transform3D Transform3D::ZERO_TRANSFORM;
