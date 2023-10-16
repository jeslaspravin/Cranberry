/*!
 * \file CommonFunctions.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#ifndef COMMONFUNCTIONS_INCLUDE
#define COMMONFUNCTIONS_INCLUDE

const vec2 inv2PiPi = vec2(0.1591, 0.3183);

// Assumes X axis is at (0.5, 0.5), +Z is +V(> 0.5), +Y > +U(>0.5)
// direction - Normalized direction
vec2 cartesianToSphericalUV(vec3 direction)
{
    vec2 retVal = vec2(atan(direction.y, direction.x), asin(direction.z));
    retVal = retVal * inv2PiPi;
    retVal += 0.5;
    return retVal;
}

// theta is from Z axis, Phi is from X axis around Y axis
vec3 sphericalToCartesian(float phi, float theta)
{
    return vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
}

#endif // COMMONFUNCTIONS_INCLUDE