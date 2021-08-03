
const vec2 invAtan = vec2(0.1591, 0.3183);

// direction - Normalized direction
vec2 cartesianToSpherical(vec3 direction)
{
    vec2 retVal = vec2(atan(direction.y, direction.x), asin(direction.z));
    retVal = retVal * invAtan;
    retVal += 0.5;
    return retVal;
}

