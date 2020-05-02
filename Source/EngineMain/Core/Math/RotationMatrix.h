#pragma once
#include "Matrix3.h"
#include "Matrix4.h"


class RotationMatrix
{
private:
    Matrix3 rotationMatrix;

public:
    RotationMatrix();
    RotationMatrix(const Matrix3& rotMatrix);
    RotationMatrix(const Matrix4& rotMatrix);
    RotationMatrix(const class Rotation& rotation);

    Rotation asRotation() const;
    const Matrix3& matrix() const;
};
