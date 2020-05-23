#pragma once
#include "../../Math/Vector3D.h"
#include "../../Math/Rotation.h"

class Matrix4;

enum class ECameraProjection
{
    Perspective,
    Orthographic
};

class Camera
{
private:
    // Field of views in degree for perspective projection
    float hFov;
    float vFov;
    // For orthographic
    Size2D orthoSize;
    float nearClip;
    float farClip;

    Vector3D location;
    Rotation rotation;

    static const float MAX_FOV;
    static const float MIN_NEAR_FAR_DIFF;
    static const float MIN_NEAR;
public:

    ECameraProjection cameraProjection;

private:
    void orthographicMatrix(Matrix4& matrix, float halfWidth, float halfHeight) const;
    void orthographicMatrix(Matrix4& matrix, float left, float right, float top, float bottom) const;
    void orthographicMatrix(Matrix4& matrix) const;

    void perspectiveMatrix(Matrix4& matrix, float halfWidth, float halfHeight) const;
    void perspectiveMatrix(Matrix4& matrix, float left, float right, float top, float bottom) const;
    void perspectiveMatrix(Matrix4& matrix) const;
public:

    void setFOV(float horizontal, float vertical);
    void setOrthoSize(const Size2D& orthographicSize);
    void setClippingPlane(float near, float far);

    void setTranslation(const Vector3D& newLocation);
    void setRotation(const Rotation& newRotation);

    void lookAt(const Vector3D& lookAtTarget);

    Matrix4 viewMatrix() const;
    Matrix4 projectionMatrix() const;
};