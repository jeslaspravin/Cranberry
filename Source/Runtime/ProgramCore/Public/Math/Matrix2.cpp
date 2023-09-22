/*!
 * \file Matrix2.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Matrix2.h"
#include "Math/Vector2.h"

const Matrix2 Matrix2::IDENTITY{ 1, 0, 0, 1 };

Matrix2::Matrix2(const Vector2 &c1, const Vector2 &c2)
    : value(c1.x(), c1.y(), c2.x(), c2.y())
{}

Matrix2::Matrix2(const Vector2 &scale)
    : value(scale.x(), 0, 0, scale.y())
{}

Vector2 Matrix2::operator* (const Vector2 &transformingVector) const { return Vector2(value * transformingVector.value); }