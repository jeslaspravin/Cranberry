/*!
 * \file HDRIToCube.comp.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../../Common/ComputeCommon.inl.glsl"
#include "../../Common/CommonFunctions.inl.glsl"

layout (set = 0, binding = 0, rgba16f) writeonly uniform imageCube outCubeMap;

layout (set = 1, binding = 0) uniform sampler2D hdri;    

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

// We are rendering like usual world view but we are assigning x to z, y to x and z to y as in vulkan cube map z is fwd x is right and y is up
// While sampling we do the same 
// And also when rendering cubemap same has to assigned to each layers
void mainComp()
{
    ivec2 outSize = imageSize(outCubeMap);
    vec2 texelUvSize = 1 / vec2(outSize);
    // Sample at center of texel
    vec2 outUV = vec2(gl_GlobalInvocationID.xy) * texelUvSize;
    outUV += texelUvSize * 0.5;

// All X's in Z faces 4, 5
// Face along +x
    vec3 direction = normalize(vec3(CUBE_COORDS[2].x, mix(CUBE_COORDS[2].yz, CUBE_COORDS[1].yz, outUV)));
    vec2 textureUV = cartesianToSphericalUV(direction);
    imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 4), texture(hdri, textureUV));
    // Debug face UV
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 4), vec4(outUV, 0, 1));    
    // Debug Texture UV per Face UV
    //imageStore(outCubeMap, ivec3(outSize * textureUV, 4), vec4(outUV, 0, 1));
    // Debug Texture UV
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 4), vec4(textureUV, 0, 1));
    
// Face along -x
    direction = normalize(vec3(CUBE_COORDS[4].x, mix(CUBE_COORDS[4].yz, CUBE_COORDS[7].yz, outUV)));
    textureUV = cartesianToSphericalUV(direction);
    imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 5), texture(hdri, textureUV));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 5), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(outSize * textureUV, 5), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 5), vec4(textureUV, 0, 1));
    
// All Y's in X faces 0,1
// Face along +y
    textureUV = mix(CUBE_COORDS[0].xz, CUBE_COORDS[5].xz, outUV);
    direction = normalize(vec3(textureUV.x, CUBE_COORDS[0].y, textureUV.y));
    textureUV = cartesianToSphericalUV(direction);
    imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 0), texture(hdri, textureUV));    
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 0), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(outSize * textureUV, 0), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 0), vec4(textureUV, 0, 1));
    
// Face along -y
    textureUV = mix(CUBE_COORDS[6].xz, CUBE_COORDS[3].xz, outUV);
    direction = normalize(vec3(textureUV.x, CUBE_COORDS[6].y, textureUV.y));
    textureUV = cartesianToSphericalUV(direction);
    imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 1), texture(hdri, textureUV));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 1), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(outSize * textureUV, 1), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 1), vec4(textureUV, 0, 1));
        
// All Z's in Y faces 2, 3
// Face along +z, flipped uv because U goes from - Y to + Y and V goes from -X to +X
    direction = normalize(vec3(mix(CUBE_COORDS[6].xy, CUBE_COORDS[0].xy, outUV.yx), CUBE_COORDS[6].z));
    textureUV = cartesianToSphericalUV(direction);
    imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 2), texture(hdri, textureUV));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 2), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(outSize * textureUV, 2), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 2), vec4(textureUV, 0, 1));
    
// Face along -z, flipped uv because U goes from - Y to + Y and V goes from +X to -X
    direction = normalize(vec3(mix(CUBE_COORDS[3].xy, CUBE_COORDS[5].xy, outUV.yx), CUBE_COORDS[3].z));
    textureUV = cartesianToSphericalUV(direction);
    imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 3), texture(hdri, textureUV));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 3), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(outSize * textureUV, 3), vec4(outUV, 0, 1));
    //imageStore(outCubeMap, ivec3(gl_GlobalInvocationID.xy, 3), vec4(textureUV, 0, 1));
}