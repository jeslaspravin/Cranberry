/*!
 * \file Transform3D.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Transform3D.h"
#include "Math/Math.h"
#include "Math/RotationMatrix.h"
#include "Math/Vector4.h"

Transform3D Transform3D::ZERO_TRANSFORM;

Transform3D::Transform3D()
    : transformTranslation(0)
    , transformScale(1)
    , transformRotation(0)
{}

Transform3D::Transform3D(const Vector3 &translation, const Rotation &rotation, const Vector3 &scale)
    : transformTranslation(translation)
    , transformScale(scale)
    , transformRotation(rotation)
{}

Transform3D::Transform3D(const Matrix4 &transformMatrix)
{
    // TODO(Jeslas) : fix cases when non uniform scaled matrix inverse creates invalid Rotation values(
    // May be quaternion will fix it )?
    transformTranslation = Vector3(transformMatrix[3].x, transformMatrix[3].y, transformMatrix[3].z);
    transformScale = Vector3(Vector3(transformMatrix[0]).length(), Vector3(transformMatrix[1]).length(), Vector3(transformMatrix[2]).length());

    Vector3 invScale(invScaleSafe());
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
    : transformTranslation(Vector3::ZERO)
    , transformScale(Vector3::ONE)
    , transformRotation(rotation)
{}

Transform3D &Transform3D::operator= (const Transform3D &otherTransform)
{
    transformTranslation = otherTransform.transformTranslation;
    transformScale = otherTransform.transformScale;
    transformRotation = otherTransform.transformRotation;

    return *this;
}

Transform3D &Transform3D::operator= (Transform3D &&otherTransform)
{
    transformTranslation = std::move(otherTransform.transformTranslation);
    transformScale = std::move(otherTransform.transformScale);
    transformRotation = std::move(otherTransform.transformRotation);

    return *this;
}

Transform3D &Transform3D::operator= (const Matrix4 &transformMatrix)
{
    transformTranslation = Vector3(transformMatrix[3].x, transformMatrix[3].y, transformMatrix[3].z);
    transformScale = Vector3(Vector3(transformMatrix[0]).length(), Vector3(transformMatrix[1]).length(), Vector3(transformMatrix[2]).length());
    transformRotation = RotationMatrix(transformMatrix / Matrix4(transformScale)).asRotation();

    return *this;
}

bool Transform3D::isSame(const Transform3D &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return transformTranslation.isSame(b.transformTranslation, epsilon) && transformRotation.isSame(b.transformRotation, epsilon)
           && transformScale.isSame(b.transformScale, epsilon);
}

const Vector3 &Transform3D::getTranslation() const { return transformTranslation; }

Vector3 &Transform3D::getTranslation() { return transformTranslation; }

const Rotation &Transform3D::getRotation() const { return transformRotation; }

Rotation &Transform3D::getRotation() { return transformRotation; }

const Vector3 &Transform3D::getScale() const { return transformScale; }

Vector3 &Transform3D::getScale() { return transformScale; }

void Transform3D::setTranslation(const Vector3 &newTranslation) { transformTranslation = newTranslation; }

void Transform3D::setRotation(const Rotation &newRotation) { transformRotation = newRotation; }

void Transform3D::setScale(const Vector3 &newScale) { transformScale = newScale; }

Vector3 Transform3D::transformNormal(const Vector3 &normal) const
{
    Vector4 transformed = normalTransformMatrix() * Vector4(normal.x(), normal.y(), normal.z(), 1);
    return Vector3(transformed.x(), transformed.y(), transformed.z());
}

Vector3 Transform3D::invTransformNormal(const Vector3 &normal) const
{
    Vector4 transformed = (normalTransformMatrix().inverse()) * Vector4(normal.x(), normal.y(), normal.z(), 1);
    return Vector3(transformed.x(), transformed.y(), transformed.z());
}

Vector3 Transform3D::transformPoint(const Vector3 &point) const
{
    // Vector4 transformed = getTransformMatrix() * Vector4(point.x(), point.y(), point.z(), 1);
    // return Vector3(transformed.x(), transformed.y(), transformed.z());
    return (RotationMatrix(transformRotation).matrix() * (transformScale * point)) + transformTranslation;
}

Vector3 Transform3D::invTransformPoint(const Vector3 &point) const
{
    // Vector4 transformed = (getTransformMatrix().inverse()) * Vector4(point.x(), point.y(),
    // point.z(), 1); return Vector3(transformed.x(), transformed.y(), transformed.z());
    return (RotationMatrix(transformRotation).matrix().transpose() * (point - transformTranslation)) * invScaleSafe();
}

Transform3D Transform3D::transform(const Transform3D &other) const { return Transform3D{ getTransformMatrix() * other.getTransformMatrix() }; }

Transform3D Transform3D::invTransform(const Transform3D &other) const
{
    return Transform3D{ inverseNonUniformScaledMatrix() * other.getTransformMatrix() };
}

Transform3D Transform3D::inverse() const
{
    Matrix3 invRot(RotationMatrix(transformRotation).matrix().transpose());
    Vector3 invScale(invScaleSafe());
    // If inversing the transform we are going reverse.
    // ie) Inverse scaling and Inverse rotating and then Inverse translating
    // So the translation has to be scaled and rotated in reverse to compensate the added inverse rotation and scaling
    return Transform3D((invRot * (invScale * -transformTranslation)), RotationMatrix(invRot).asRotation(), invScale);
}

Transform3D Transform3D::inverseNonUniformScaled() const
{
    if (Math::isEqual(transformScale.x(), transformScale.y()) && Math::isEqual(transformScale.x(), transformScale.z()))
    {
        return inverse();
    }
    return Transform3D(inverseNonUniformScaledMatrix());
}

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

Matrix4 Transform3D::inverseNonUniformScaledMatrix() const
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

    Matrix4 invTranslationMatrx(Vector3::ONE);
    invTranslationMatrx[3] = Matrix4Col(-transformTranslation.x(), -transformTranslation.y(), -transformTranslation.z(), 1.0f);
    return transformMatrix * invTranslationMatrx;
}

Vector3 Transform3D::invScaleSafe() const { return transformScale.safeInverse(); }

Vector3 Transform3D::invTranslation() const
{
    return Vector3(
        Math::isEqual(transformTranslation.x(), 0.0f) ? 0.0f : -transformTranslation.x(),
        Math::isEqual(transformTranslation.y(), 0.0f) ? 0.0f : -transformTranslation.y(),
        Math::isEqual(transformTranslation.z(), 0.0f) ? 0.0f : -transformTranslation.z()
    );
}