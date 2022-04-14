/*!
 * \file Camera.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Math/Matrix4.h"
#include "Math/Rotation.h"
#include "Math/Vector3D.h"
#include "ProgramCoreExports.h"

#include <optional>

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
    Size2D orthoSize;
    float nearClip;
    float farClip;
    std::optional<Matrix4> customProjMatrix;

    Vector3D camTranslation;
    Rotation camRotation;

    static const float MAX_FOV;
    static const float MIN_NEAR_FAR_DIFF;
    static const float MIN_NEAR;

public:
    ECameraProjection cameraProjection;

private:
    void orthographicMatrix(Matrix4 &matrix, float halfWidth, float halfHeight) const;
    void orthographicMatrix(Matrix4 &matrix, float left, float right, float top, float bottom) const;
    void orthographicMatrix(Matrix4 &matrix) const;

    void perspectiveMatrix(Matrix4 &matrix, float halfWidth, float halfHeight) const;
    void perspectiveMatrix(Matrix4 &matrix, float left, float right, float top, float bottom) const;
    void perspectiveMatrix(Matrix4 &matrix) const;

public:
    void setFOV(float horizontal, float vertical);
    void setOrthoSize(const Size2D &orthographicSize);
    void setClippingPlane(float near, float far);

    void setCustomProjection(Matrix4 projMatrix);
    void clearCustomProjection();

    void setTranslation(const Vector3D &newLocation);
    Vector3D translation() const { return camTranslation; }
    void setRotation(const Rotation &newRotation);
    Rotation rotation() const { return camRotation; }
    float farPlane() const { return farClip; }
    float nearPlane() const { return nearClip; }
    void frustumCorners(Vector3D *corners, Vector3D *center = nullptr) const;

    void lookAt(const Vector3D &lookAtTarget);
    // Expected pos input
    // (0,0) ----------------
    //      |                |
    //      |                |
    //      |                |
    //       ----------------  (1, 1)
    Vector3D screenToWorld(const Vector2D &screenPos) const;
    Vector3D screenToWorldFwd(const Vector2D &screenPos) const;

    Matrix4 viewMatrix() const;
    Matrix4 projectionMatrix() const;
};