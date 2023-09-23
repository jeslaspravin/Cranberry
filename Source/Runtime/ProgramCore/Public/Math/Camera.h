/*!
 * \file Camera.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "Math/Matrix4.h"
#include "Math/Rotation.h"
#include "Math/Vector3.h"
#include "Types/Containers/ArrayView.h"

#include <optional>

class Plane;

enum class PROGRAMCORE_EXPORT ECameraProjection
{
    Perspective,
    Orthographic
};

class PROGRAMCORE_EXPORT Camera
{
private:
    // Field of views in degree for perspective projection
    float hFov;
    float vFov;
    // For orthographic
    UInt2 orthoSize;
    float nearClip;
    float farClip;
    std::optional<Matrix4> customProjMatrix;

    Vector3 camTranslation;
    Rotation camRotation;

    static const float MAX_FOV;
    static const float MIN_NEAR_FAR_DIFF;
    static const float MIN_NEAR;

public:
    ECameraProjection cameraProjection;

    enum EFrustumPlane
    {
        FPlane_Left = 0,
        FPlane_Right,
        FPlane_Bottom,
        FPlane_Top,
        FPlane_Near,
        FPlane_Far,
    };

private:
    void orthographicMatrix(Matrix4 &matrix, float halfWidth, float halfHeight) const;
    void orthographicMatrix(Matrix4 &matrix, float left, float right, float top, float bottom) const;
    void orthographicMatrix(Matrix4 &matrix) const;

    void perspectiveMatrix(Matrix4 &matrix, float halfWidth, float halfHeight) const;
    void perspectiveMatrix(Matrix4 &matrix, float left, float right, float top, float bottom) const;
    void perspectiveMatrix(Matrix4 &matrix) const;

public:
    void setFOV(float horizontal, float vertical);
    void setOrthoSize(const UInt2 &orthographicSize);
    void setClippingPlane(float near, float far);

    void setCustomProjection(Matrix4 projMatrix);
    void clearCustomProjection();

    void setTranslation(const Vector3 &newLocation);
    Vector3 translation() const { return camTranslation; }
    void setRotation(const Rotation &newRotation);
    Rotation rotation() const { return camRotation; }
    float farPlane() const { return farClip; }
    float nearPlane() const { return nearClip; }
    void frustumCorners(ArrayRange<Vector3> corners, Vector3 *center = nullptr) const;
    // use EFrustumPlane to index the exact plane
    void frustumPlanes(ArrayRange<Plane> planes) const;

    void lookAt(const Vector3 &lookAtTarget);
    // Expected pos input
    // (0,0) ----------------
    //      |                |
    //      |                |
    //      |                |
    //       ----------------  (1, 1)
    Vector3 screenToWorld(const Vector2 &screenPos) const;
    Vector3 screenToWorldFwd(const Vector2 &screenPos) const;

    Matrix4 viewMatrix() const;
    Matrix4 viewMatrix(Matrix4 &outInvView) const;
    Matrix4 projectionMatrix() const;
};