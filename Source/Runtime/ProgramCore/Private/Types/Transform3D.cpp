/*!
 * \file Transform3D.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Transform3D.h"
#include "Math/Math.h"
#include "Math/RotationMatrix.h"
#include "Math/Vector4D.h"

Transform3D::Transform3D()
    : transformTranslation(0)
    , transformScale(1)
    , transformRotation(0)
{}

Transform3D::Transform3D(const Vector3D &translation, const Rotation &rotation, const Vector3D &scale)
    : transformTranslation(translation)
    , transformScale(scale)
    , transformRotation(rotation)
{}

Transform3D::Transform3D(const Matrix4 &transformMatrix)
{
    // TODO(Jeslas) : fix cases when non uniform scaled matrix inverse creates invalid Rotation values(
    // May be quaternion will fix it )?
    transformTranslation = Vector3D(transformMatrix[3].x, transformMatrix[3].y, transformMatrix[3].z);
    transformScale
        = Vector3D(Vector3D(transformMatrix[0]).length(), Vector3D(transformMatrix[1]).length(), Vector3D(transformMatrix[2]).length());

    Vector3D invScale(invScaleSafe());
    Matrix3 rotMatrix;
    rotMatrix[0] = transformMatrix[0] * (invScale[0] == 0.0f ? 1.0f : invScale[0]);
    rotMatrix[1] = transformMatrix[1] * (invScale[1] == 0.0f ? 1.0f : invScale[1]);
    rotMatrix[2] = transformMatrix[2] * (invScale[2] == 0.0f ? 1.0f : invScale[2]);
    if (rotMatrix.determinant() < 0.0f)
    {
        // Invert x axis so that negative scaling gives correct value as well
        transformScale.x() *= -1;
        rotMatrix[0] *= -1;
    }
    RotationMatrix rotMat(rotMatrix);
    rotMat.orthogonalize();
    transformRotation = rotMat.asRotation();
}

Transform3D::Transform3D(const Transform3D &otherTransform)
    : transformTranslation(otherTransform.transformTranslation)
    , transformScale(otherTransform.transformScale)
    , transformRotation(otherTransform.transformRotation)
{}

Transform3D::Transform3D(Transform3D &&otherTransform)
    : transformTranslation(std::move(otherTransform.transformTranslation))
    , transformScale(std::move(otherTransform.transformScale))
    , transformRotation(std::move(otherTransform.transformRotation))
{}

Transform3D::Transform3D(const Rotation &rotation)
    : transformTranslation(Vector3D::ZERO)
    , transformScale(Vector3D::ONE)
    , transformRotation(rotation)
{}

void Transform3D::operator=(const Transform3D &otherTransform)
{
    transformTranslation = otherTransform.transformTranslation;
    transformScale = otherTransform.transformScale;
    transformRotation = otherTransform.transformRotation;
}

void Transform3D::operator=(Transform3D &&otherTransform)
{
    transformTranslation = std::move(otherTransform.transformTranslation);
    transformScale = std::move(otherTransform.transformScale);
    transformRotation = std::move(otherTransform.transformRotation);
}

void Transform3D::operator=(const Matrix4 &transformMatrix)
{
    transformTranslation = Vector3D(transformMatrix[3].x, transformMatrix[3].y, transformMatrix[3].z);
    transformScale
        = Vector3D(Vector3D(transformMatrix[0]).length(), Vector3D(transformMatrix[1]).length(), Vector3D(transformMatrix[2]).length());
    transformRotation = RotationMatrix(transformMatrix / Matrix4(transformScale)).asRotation();
}

const Vector3D &Transform3D::getTranslation() const { return transformTranslation; }

Vector3D &Transform3D::getTranslation() { return transformTranslation; }

const Rotation &Transform3D::getRotation() const { return transformRotation; }

Rotation &Transform3D::getRotation() { return transformRotation; }

const Vector3D &Transform3D::getScale() const { return transformScale; }

Vector3D &Transform3D::getScale() { return transformScale; }

void Transform3D::setTranslation(const Vector3D &newTranslation) { transformTranslation = newTranslation; }

void Transform3D::setRotation(const Rotation &newRotation) { transformRotation = newRotation; }

void Transform3D::setScale(const Vector3D &newScale) { transformScale = newScale; }

Matrix4 Transform3D::normalTransformMatrix() const
{
    Matrix4 normTranform;
    RotationMatrix rotMatrix(transformRotation);
    normTranform[0] = Matrix4Col(rotMatrix.matrix()[0], 0.0f);
    normTranform[1] = Matrix4Col(rotMatrix.matrix()[1], 0.0f);
    normTranform[2] = Matrix4Col(rotMatrix.matrix()[2], 0.0f);
    // Inversing scale alone
    normTranform *= Matrix4(invScaleSafe());
    return normTranform;
}

Matrix4 Transform3D::getTransformMatrix() const
{
    Matrix4 transformMatrix;
    // Scales -> Rotate -> Translate all in transform existing space
    RotationMatrix rotMatrix(transformRotation);
    transformMatrix[0] = Matrix4Col(rotMatrix.matrix()[0], 0.0f);
    transformMatrix[1] = Matrix4Col(rotMatrix.matrix()[1], 0.0f);
    transformMatrix[2] = Matrix4Col(rotMatrix.matrix()[2], 0.0f);

    transformMatrix *= Matrix4(transformScale);

    transformMatrix[3] = Matrix4Col(transformTranslation.x(), transformTranslation.y(), transformTranslation.z(), 1.0);

    return transformMatrix;
}

Vector3D Transform3D::invScaleSafe() const
{
    return Vector3D(
        Math::isEqual(transformScale.x(), 0.0f) ? 0.0f : 1.0f / transformScale.x(),
        Math::isEqual(transformScale.y(), 0.0f) ? 0.0f : 1.0f / transformScale.y(),
        Math::isEqual(transformScale.z(), 0.0f) ? 0.0f : 1.0f / transformScale.z()
    );
}

Vector3D Transform3D::invTranslation() const
{
    return Vector3D(
        Math::isEqual(transformTranslation.x(), 0.0f) ? 0.0f : -transformTranslation.x(),
        Math::isEqual(transformTranslation.y(), 0.0f) ? 0.0f : -transformTranslation.y(),
        Math::isEqual(transformTranslation.z(), 0.0f) ? 0.0f : -transformTranslation.z()
    );
}

Vector3D Transform3D::transformNormal(const Vector3D &normal) const
{
    Vector4D transformed = normalTransformMatrix() * Vector4D(normal.x(), normal.y(), normal.z(), 1);
    return Vector3D(transformed.x(), transformed.y(), transformed.z());
}

Vector3D Transform3D::invTransformNormal(const Vector3D &normal) const
{
    Vector4D transformed = (normalTransformMatrix().inverse()) * Vector4D(normal.x(), normal.y(), normal.z(), 1);
    return Vector3D(transformed.x(), transformed.y(), transformed.z());
}

Vector3D Transform3D::transformPoint(const Vector3D &point) const
{
    // Vector4D transformed = getTransformMatrix() * Vector4D(point.x(), point.y(), point.z(), 1);
    // return Vector3D(transformed.x(), transformed.y(), transformed.z());
    return (RotationMatrix(transformRotation).matrix() * (transformScale * point)) + transformTranslation;
}

Vector3D Transform3D::invTransformPoint(const Vector3D &point) const
{
    // Vector4D transformed = (getTransformMatrix().inverse()) * Vector4D(point.x(), point.y(),
    // point.z(), 1); return Vector3D(transformed.x(), transformed.y(), transformed.z());
    return (RotationMatrix(transformRotation).matrix().transpose() * (point - transformTranslation)) * invScaleSafe();
}

Transform3D Transform3D::transform(const Transform3D &other) { return Transform3D{ getTransformMatrix() * other.getTransformMatrix() }; }

Transform3D Transform3D::invTransform(const Transform3D &other)
{
    return Transform3D{ (getTransformMatrix().inverse()) * other.getTransformMatrix() };
}

Transform3D Transform3D::inverse() const
{
    Matrix3 invRot(RotationMatrix(transformRotation).matrix().transpose());
    Vector3D invScale(invScaleSafe());
    return Transform3D((invRot * (invScale * -transformTranslation)), RotationMatrix(invRot).asRotation(), invScale);
}

Transform3D Transform3D::inverseNonUniformScaled() const
{
    // (Translate * Rotate * Scale) ^ -1 == InvScale * InvRotate * InvTranslate
    Matrix3 invRot(RotationMatrix(transformRotation).matrix().transpose());
    Matrix3 invScale(invScaleSafe());
    invScale *= invRot;
    Matrix4 transformMatrix;
    transformMatrix[0] = Matrix4Col(invScale[0], 0.0f);
    transformMatrix[1] = Matrix4Col(invScale[1], 0.0f);
    transformMatrix[2] = Matrix4Col(invScale[2], 0.0f);
    transformMatrix[3][3] = 1.0f;

    Matrix4 invTranslationMatrx(Vector3D::ONE);
    invTranslationMatrx[3] = Matrix4Col(-transformTranslation.x(), -transformTranslation.y(), -transformTranslation.z(), 1.0f);

    return Transform3D(transformMatrix * invTranslationMatrx);
}

Transform3D Transform3D::ZERO_TRANSFORM;
