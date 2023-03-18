/*!
 * \file EnvToDiffuseIrradiance.comp.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../../Common/ComputeCommon.inl.glsl"
#include "../../Common/CommonDefines.inl.glsl"
#include "../../Common/CommonFunctions.inl.glsl"

layout(constant_id = 4) const uint SAMPLE_COUNT = 128;

layout (set = 0, binding = 0, rgba32f) writeonly uniform imageCube outDiffuseIrradiance;

layout (set = 1, binding = 0) uniform samplerCube envMap;

//layout(push_constant) uniform Constants
//{
//    float time;
//    uint flags;
//} constants;

const vec3 CUBE_COORDS[8] = 
{
        vec3( 50,  50,  50)
    ,    vec3( 50,  50, -50)
    ,    vec3( 50, -50,  50)
    ,    vec3( 50, -50, -50)
    ,    vec3(-50,  50,  50)
    ,    vec3(-50,  50, -50)
    ,    vec3(-50, -50,  50)
    ,    vec3(-50, -50, -50)
};

vec3 calculateIrrad(vec3 direction)
{
    vec3 diffuseIrrad = vec3(0.0);

    vec3 bitan = normalize(cross(direction, (abs(direction.x) < 0.999? vec3(1,0,0) : vec3(0,0,1))));
    vec3 tangent = normalize(cross(bitan, direction));

    int nSamples = 0;
    // In radians
    const float sampleDelta = M_PI / float(SAMPLE_COUNT);

    for(float phi = 0.0; phi < 2.0 * M_PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * M_PI; theta += sampleDelta)
        {
            vec3 sampleDir = sphericalToCartesian(phi, theta);
            sampleDir = (sampleDir.x * tangent + sampleDir.y * bitan + sampleDir.z * direction);

            diffuseIrrad += texture(envMap, ENGINE_WORLD_TO_CUBE_DIR(sampleDir)).xyz * sin(theta) * cos(theta);

            nSamples++;
        }
    }

    diffuseIrrad *= M_PI / float(nSamples);
    return diffuseIrrad;
}

// We are rendering like usual world view but we are assigning x to z, y to x and z to y as in vulkan cube map z is fwd x is right and y is up
// While sampling we do the same 
// And also when rendering cubemap same has to assigned to each layers
void mainComp()
{
    ivec2 outSize = imageSize(outDiffuseIrradiance);
    vec2 texelUvSize = 1 / vec2(outSize);
    // Sample at center of texel
    vec2 outUV = vec2(gl_GlobalInvocationID.xy) * texelUvSize;
    outUV += texelUvSize * 0.5;
    

    vec3 diffuseIrrad;
// All X's in Z faces 4, 5
// Face along +x
    vec3 direction = normalize(vec3(CUBE_COORDS[2].x, mix(CUBE_COORDS[2].yz, CUBE_COORDS[1].yz, outUV)));
    diffuseIrrad = calculateIrrad(direction);
    imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 4), vec4(diffuseIrrad, 1.0));
    
// Face along -x
    direction = normalize(vec3(CUBE_COORDS[4].x, mix(CUBE_COORDS[4].yz, CUBE_COORDS[7].yz, outUV)));
    diffuseIrrad = calculateIrrad(direction);
    imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 5), vec4(diffuseIrrad, 1.0));
    
// All Y's in X faces 0,1
// Face along +y
    vec2 faceSpaceUV = mix(CUBE_COORDS[0].xz, CUBE_COORDS[5].xz, outUV);
    direction = normalize(vec3(faceSpaceUV.x, CUBE_COORDS[0].y, faceSpaceUV.y));
    diffuseIrrad = calculateIrrad(direction);
    imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 0), vec4(diffuseIrrad, 1.0));
    
// Face along -y
    faceSpaceUV = mix(CUBE_COORDS[6].xz, CUBE_COORDS[3].xz, outUV);
    direction = normalize(vec3(faceSpaceUV.x, CUBE_COORDS[6].y, faceSpaceUV.y));
    diffuseIrrad = calculateIrrad(direction);
    imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 1), vec4(diffuseIrrad, 1.0));
        
// All Z's in Y faces 2, 3
// Face along +z, flipped uv because U goes from - Y to + Y and V goes from -X to +X
    direction = normalize(vec3(mix(CUBE_COORDS[6].xy, CUBE_COORDS[0].xy, outUV.yx), CUBE_COORDS[6].z));
    diffuseIrrad = calculateIrrad(direction);
    imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 2), vec4(diffuseIrrad, 1.0));
    
// Face along -z, flipped uv because U goes from - Y to + Y and V goes from +X to -X
    direction = normalize(vec3(mix(CUBE_COORDS[3].xy, CUBE_COORDS[5].xy, outUV.yx), CUBE_COORDS[3].z));
    diffuseIrrad = calculateIrrad(direction);
    imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 3), vec4(diffuseIrrad, 1.0));
}