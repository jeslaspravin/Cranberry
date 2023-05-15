/*!
 * \file Rotation.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Rotation.h"
#include "Math/Math.h"

bool Rotation::isSame(const Rotation &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
           && Math::isEqual(value.z, b.value.z, epsilon);
}

bool Rotation::isFinite() const { return Math::isFinite(value.x) && Math::isFinite(value.y) && Math::isFinite(value.z); }

Rotation Math::deg2Rad(const Rotation &value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::deg2Rad(rotVal);
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::rad2Deg(const Rotation &value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::rad2Deg(rotVal);
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::sin(const Rotation &value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::sin(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::cos(const Rotation &value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::cos(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::tan(const Rotation &value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::tan(Math::deg2Rad(rotVal));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::asin(const Rotation &value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::rad2Deg(Math::asin(Math::deg2Rad(rotVal)));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::acos(const Rotation &value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::rad2Deg(Math::acos(Math::deg2Rad(rotVal)));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}

Rotation Math::atan(const Rotation &value)
{
    glm::vec3 rotVal(value.roll(), value.pitch(), value.yaw());
    rotVal = Math::rad2Deg(Math::atan(Math::deg2Rad(rotVal)));
    return Rotation(rotVal.x, rotVal.y, rotVal.z);
}
