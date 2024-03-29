/*!
 * \file Camera.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Camera.h"
#include "Math/Math.h"
#include "Math/RotationMatrix.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Math/Transform3D.h"
#include "Math/Quaternion.h"
#include "Math/Plane.h"

const float Camera::MAX_FOV(175.f);
const float Camera::MIN_NEAR_FAR_DIFF(1.f);
const float Camera::MIN_NEAR(0.1f);

void Camera::orthographicMatrix(Matrix4 &matrix, float halfWidth, float halfHeight) const
{
    const float nMinusfInv = 1 / (nearClip - farClip);

    //  Matrix arrangement(Transpose arrangement in memory)
    //  r0(c0)  r1(c0)  r2(c0)  r3(c0)
    //  r0(c1)  r1(c1)  r2(c1)  r3(c1)
    //  r0(c2)  r1(c2)  r2(c2)  r3(c2)
    //  r0(c3)  r1(c3)  r2(c3)  r3(c3)
    matrix = Matrix4(1 / halfWidth, 0, 0, 0, 0, 1 / halfHeight, 0, 0, 0, 0, nMinusfInv, 0, 0, 0, -farClip * nMinusfInv, 1);
}

void Camera::orthographicMatrix(Matrix4 &matrix, float left, float right, float top, float bottom) const
{
    const float nMinusfInv = 1 / (nearClip - farClip);
    const float rMinuslInv = 1 / (right - left);
    const float bMinustInv = 1 / (bottom - top);

    //  Matrix arrangement(Transpose arrangement in memory)
    //  r0(c0)  r1(c0)  r2(c0)  r3(c0)
    //  r0(c1)  r1(c1)  r2(c1)  r3(c1)
    //  r0(c2)  r1(c2)  r2(c2)  r3(c2)
    //  r0(c3)  r1(c3)  r2(c3)  r3(c3)
    matrix = Matrix4(
        2 * rMinuslInv, 0, 0, 0, 0, 2 * bMinustInv, 0, 0, 0, 0, nMinusfInv, 0, -(right + left) * rMinuslInv, -(bottom + top) * bMinustInv,
        -farClip * nMinusfInv, 1
    );
}

void Camera::orthographicMatrix(Matrix4 &matrix) const { orthographicMatrix(matrix, orthoSize.x * 0.5f, orthoSize.y * 0.5f); }

void Camera::perspectiveMatrix(Matrix4 &matrix, float halfWidth, float halfHeight) const
{
    const float nMinusfInv = 1 / (nearClip - farClip);

    //  Matrix arrangement(Transpose arrangement in memory)
    //  r0(c0)  r1(c0)  r2(c0)  r3(c0)
    //  r0(c1)  r1(c1)  r2(c1)  r3(c1)
    //  r0(c2)  r1(c2)  r2(c2)  r3(c2)
    //  r0(c3)  r1(c3)  r2(c3)  r3(c3)
    matrix = Matrix4(
        nearClip / halfWidth, 0, 0, 0, 0, nearClip / halfHeight, 0, 0, 0, 0, nearClip * nMinusfInv, 1, 0, 0, -nearClip * farClip * nMinusfInv, 0
    );
}

void Camera::perspectiveMatrix(Matrix4 &matrix, float left, float right, float top, float bottom) const
{
    const float nMinusfInv = 1 / (nearClip - farClip);
    const float rMinuslInv = 1 / (right - left);
    const float bMinustInv = 1 / (bottom - top);

    //  Matrix arrangement(Transpose arrangement in memory)
    //  r0(c0)  r1(c0)  r2(c0)  r3(c0)
    //  r0(c1)  r1(c1)  r2(c1)  r3(c1)
    //  r0(c2)  r1(c2)  r2(c2)  r3(c2)
    //  r0(c3)  r1(c3)  r2(c3)  r3(c3)
    matrix = Matrix4(
        2 * nearClip * rMinuslInv, 0, 0, 0, 0, 2 * nearClip * bMinustInv, 0, 0, -(right + left) * rMinuslInv, -(bottom + top) * bMinustInv,
        nearClip * nMinusfInv, 1, 0, 0, -nearClip * farClip * nMinusfInv, 0
    );
}

void Camera::perspectiveMatrix(Matrix4 &matrix) const
{
    float halfWidth = Math::tan(Math::deg2Rad(hFov * 0.5f)) * nearClip;
    float halfHeight = Math::tan(Math::deg2Rad(vFov * 0.5f)) * nearClip;

    perspectiveMatrix(matrix, halfWidth, halfHeight);
}

void Camera::setFOV(float horizontal, float vertical)
{
    hFov = Math::min(Math::abs(horizontal), MAX_FOV);
    vFov = Math::min(Math::abs(vertical), MAX_FOV);
}

void Camera::setOrthoSize(const UInt2 &orthographicSize) { orthoSize = orthographicSize; }

void Camera::setClippingPlane(float near, float far)
{
    nearClip = Math::max(near, MIN_NEAR);
    farClip = Math::max(far, nearClip + MIN_NEAR_FAR_DIFF);
}

void Camera::setCustomProjection(Matrix4 projMatrix) { customProjMatrix = projMatrix; }

void Camera::clearCustomProjection() { customProjMatrix.reset(); }

void Camera::setTranslation(const Vector3 &newLocation) { camTranslation = newLocation; }

void Camera::setRotation(const Rotation &newRotation) { camRotation = newRotation; }

void Camera::frustumCorners(ArrayRange<Vector3> corners, Vector3 *center /*= nullptr*/) const
{
    Matrix4 ndcToWorld = viewMatrix() * projectionMatrix().inverse();
    Vector3 frustumMid(0);
    int32 cornerIdx = 0;
    for (float z = 0; z < 2; ++z)
    {
        for (float y = -1; y < 2; y += 2)
        {
            for (float x = -1; x < 2; x += 2)
            {
                Vector4 worldPos(ndcToWorld * Vector4(x, y, z, 1));
                worldPos /= worldPos.w();

                corners[cornerIdx] = Vector3(worldPos);
                frustumMid += corners[cornerIdx];
                cornerIdx++;
            }
        }
    }
    if (center)
    {
        *center = frustumMid / float(cornerIdx);
    }
}

void Camera::frustumPlanes(ArrayRange<Plane> planes) const
{
    // Gribb-Hartmann Plane extraction from Frustum
    // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

    Matrix4 worldToClip;
    viewMatrix(worldToClip); // get inverted view matrix
    worldToClip = projectionMatrix() * worldToClip;

    // Transpose to address Rows directly
    worldToClip = worldToClip.transpose();

    // Left => V.(R0 + R3) >= 0
    planes[FPlane_Left] = Plane{ worldToClip[3] + worldToClip[0] };
    planes[FPlane_Left].normalized();
    // Right => V.(R3 - R0) >= 0
    planes[FPlane_Right] = Plane{ worldToClip[3] - worldToClip[0] };
    planes[FPlane_Right].normalized();
    // Bottom => V.(R1 + R3) >= 0
    planes[FPlane_Bottom] = Plane{ worldToClip[3] + worldToClip[1] };
    planes[FPlane_Bottom].normalized();
    // Top => V.(R3 - R1) >= 0
    planes[FPlane_Top] = Plane{ worldToClip[3] - worldToClip[1] };
    planes[FPlane_Top].normalized();
    // Near => V.R2 >= 0
    planes[FPlane_Near] = Plane{ worldToClip[2] };
    planes[FPlane_Near].normalized();
    // Far => V.(R3 - R2) >= 0
    planes[FPlane_Far] = Plane{ worldToClip[3] - worldToClip[2] };
    planes[FPlane_Far].normalized();
}

void Camera::lookAt(const Vector3 &lookAtTarget)
{
    RotationMatrix rotMatrix = RotationMatrix::fromX(lookAtTarget - camTranslation);
    setRotation(rotMatrix.asRotation());
}

Vector3 Camera::screenToWorld(const Vector2 &screenPos) const
{
    // Fliping y since Quad draw uses vulkan screen coord top left -1,-1 bottom right 1,1. But our
    // view/projection y coordinate is from bottom(-1) to top(1)
    Vector4 ndcCoord{ ((screenPos.x() - 0.5f) * 2), (-(screenPos.y() - 0.5f) * 2), 1, 1 };
    Vector4 worldCoord = projectionMatrix().inverse() * ndcCoord;
    worldCoord /= worldCoord.w();
    worldCoord = viewMatrix() * worldCoord;
    return { worldCoord };
}

Vector3 Camera::screenToWorldFwd(const Vector2 &screenPos) const { return (screenToWorld(screenPos) - camTranslation).safeNormalized(); }

Matrix4 Camera::viewMatrix() const
{
    Transform3D transform;
    transform.setRotation(camRotation);
    transform.setTranslation(camTranslation);

    // Since in view space fwd is Z axis so cyclically rotating axis to make real fwd as up
    Matrix4 tfMatrix = transform.getTransformMatrix();
    Matrix4 viewMat;
    viewMat[0] = tfMatrix[1];
    viewMat[1] = tfMatrix[2];
    viewMat[2] = tfMatrix[0];
    viewMat[3] = tfMatrix[3];

    return viewMat;
}

Matrix4 Camera::viewMatrix(Matrix4 &outInvView) const
{
    Matrix4 viewMat = viewMatrix();
    Matrix3 viewInv = { Vector3(viewMat[0]), Vector3(viewMat[1]), Vector3(viewMat[2]) };
    viewInv = viewInv.transpose();
    outInvView = { Vector3(viewInv[0]), Vector3(viewInv[1]), Vector3(viewInv[2]), -(viewInv * Vector3(viewMat[3])) };
    return viewMat;
}

Matrix4 Camera::projectionMatrix() const
{
    if (customProjMatrix)
    {
        return *customProjMatrix;
    }

    Matrix4 projectionMat = Matrix4::IDENTITY;

    switch (cameraProjection)
    {
    case ECameraProjection::Perspective:
        perspectiveMatrix(projectionMat);
        break;
    case ECameraProjection::Orthographic:
        orthographicMatrix(projectionMat);
        break;
    default:
        break;
    }
    return projectionMat;
}
