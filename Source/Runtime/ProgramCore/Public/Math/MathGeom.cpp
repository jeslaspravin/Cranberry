/*!
 * \file MathGeom.cpp
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/MathGeom.h"

Vector2 MathGeom::transform2d(const Vector2 &pt, const Vector2 &offset, float rotInDeg)
{
    rotInDeg = Math::deg2Rad(rotInDeg);
    float rotSin = Math::sin(rotInDeg), rotCos = Math::cos(rotInDeg);

    return Vector2(pt.x() * rotCos - pt.y() * rotSin, pt.x() * rotSin + pt.y() * rotCos) + offset;
}
