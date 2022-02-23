/*!
 * \file MathGeom.cpp
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/MathGeom.h"

Vector2D MathGeom::transform2d(const Vector2D& pt, const Vector2D& offset, float rotInDeg)
{
    rotInDeg = Math::deg2Rad(rotInDeg);
    float rotSin = Math::sin(rotInDeg), rotCos = Math::cos(rotInDeg);

    return Vector2D(
        pt.x() * rotCos - pt.y() * rotSin
        , pt.x() * rotSin + pt.y() * rotCos) + offset;
}
