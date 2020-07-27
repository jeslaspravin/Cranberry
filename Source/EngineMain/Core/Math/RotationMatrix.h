#pragma once
#include "Matrix3.h"
#include "Matrix4.h"


class RotationMatrix
{
private:
    Matrix3 rotationMatrix;
    inline void fromRotation(const class Rotation& rotation);
public:
    RotationMatrix();
    RotationMatrix(const RotationMatrix& other);
    RotationMatrix(RotationMatrix&& other);
    RotationMatrix(const Matrix3& rotMatrix);
    RotationMatrix(const Matrix4& rotMatrix);
    RotationMatrix(const class Rotation& rotation);

    Rotation asRotation() const;
    const Matrix3& matrix() const;

    void orthogonalize();

    static RotationMatrix fromX(const Vector3D& x);
    static RotationMatrix fromY(const Vector3D& y);
    static RotationMatrix fromZ(const Vector3D& z);
    static RotationMatrix fromXY(const Vector3D& x, const Vector3D& y);
    static RotationMatrix fromYZ(const Vector3D& y, const Vector3D& z);
    static RotationMatrix fromZX(const Vector3D& z, const Vector3D& x);
    static RotationMatrix fromXYZ(const Vector3D& x, const Vector3D& y, const Vector3D& z);
};
