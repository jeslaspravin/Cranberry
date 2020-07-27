#include "Camera.h"
#include "../../Math/Matrix4.h"
#include "../Transform3D.h"
#include "../../Math/RotationMatrix.h"
#include "../../Math/Math.h"
#include "../../../RenderInterface/GlobalRenderVariables.h"
#include "../../Math/Vector2D.h"

const float Camera::MAX_FOV(175.f);
const float Camera::MIN_NEAR_FAR_DIFF(1.f);
const float Camera::MIN_NEAR(1.f);

void Camera::orthographicMatrix(Matrix4& matrix, float halfWidth, float halfHeight) const
{
    const float nMinusfInv = 1 / (nearClip - farClip);

    //  Matrix arrangement(Transpose arrangement in memory)
    //  r0(c0)  r1(c0)  r2(c0)  r3(c0)
    //  r0(c1)  r1(c1)  r2(c1)  r3(c1)
    //  r0(c2)  r1(c2)  r2(c2)  r3(c2)
    //  r0(c3)  r1(c3)  r2(c3)  r3(c3)
    matrix = Matrix4(
        1 / halfWidth, 0, 0, 0,
        0, 1 / halfHeight, 0, 0,
        0, 0, nMinusfInv, 0,
        0, 0, -farClip * nMinusfInv, 1
    );
}

void Camera::orthographicMatrix(Matrix4& matrix, float left, float right, float top, float bottom) const
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
        2 * rMinuslInv, 0, 0, 0,
        0, 2 * bMinustInv, 0, 0,
        0, 0, nMinusfInv, 0,
        -(right + left) * rMinuslInv, -(bottom + top) * bMinustInv, -farClip * nMinusfInv, 1
    );
}

void Camera::orthographicMatrix(Matrix4& matrix) const
{
    orthographicMatrix(matrix, orthoSize.x * 0.5f, orthoSize.y * 0.5f);
}

void Camera::perspectiveMatrix(Matrix4& matrix, float halfWidth, float halfHeight) const
{
    const float nMinusfInv = 1 / (nearClip - farClip);

    //  Matrix arrangement(Transpose arrangement in memory)
    //  r0(c0)  r1(c0)  r2(c0)  r3(c0)
    //  r0(c1)  r1(c1)  r2(c1)  r3(c1)
    //  r0(c2)  r1(c2)  r2(c2)  r3(c2)
    //  r0(c3)  r1(c3)  r2(c3)  r3(c3)
    matrix = Matrix4(
        nearClip / halfWidth, 0, 0, 0,
        0, nearClip / halfHeight, 0, 0,
        0, 0, nearClip * nMinusfInv, 1,
        0, 0, -nearClip * farClip * nMinusfInv, 0
    );
}

void Camera::perspectiveMatrix(Matrix4& matrix, float left, float right, float top, float bottom) const
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
        2 * nearClip * rMinuslInv, 0, 0, 0,
        0, 2 * nearClip * bMinustInv, 0, 0,
        -(right + left) * rMinuslInv, -(bottom + top) * bMinustInv, nearClip * nMinusfInv, 1,
        0, 0, -nearClip * farClip * nMinusfInv, 0
    );
}

void Camera::perspectiveMatrix(Matrix4& matrix) const
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

void Camera::setOrthoSize(const Size2D& orthographicSize)
{
    orthoSize = orthographicSize;
}

void Camera::setClippingPlane(float near, float far)
{
    nearClip = Math::max(near, MIN_NEAR);
    farClip = Math::max(far, nearClip + MIN_NEAR_FAR_DIFF);
}

void Camera::setTranslation(const Vector3D& newLocation)
{
    camTranslation = newLocation;
}

void Camera::setRotation(const Rotation& newRotation)
{
    camRotation = newRotation;
}

void Camera::lookAt(const Vector3D& lookAtTarget)
{
    RotationMatrix rotMatrix = RotationMatrix::fromX(lookAtTarget - camTranslation);
    setRotation(rotMatrix.asRotation());
}

Matrix4 Camera::viewMatrix() const
{
    Transform3D transform;
    transform.setRotation(camRotation);
    transform.setTranslation(camTranslation);

    // Since in view space fwd is Z axis so cyclically rotating axis to make real fwd as up
    Matrix4 viewMat;
    viewMat[0] = transform.getTransformMatrix()[1];
    viewMat[1] = transform.getTransformMatrix()[2];
    viewMat[2] = transform.getTransformMatrix()[0];
    viewMat[3] = transform.getTransformMatrix()[3];

    return viewMat;
}

Matrix4 Camera::projectionMatrix() const
{
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
