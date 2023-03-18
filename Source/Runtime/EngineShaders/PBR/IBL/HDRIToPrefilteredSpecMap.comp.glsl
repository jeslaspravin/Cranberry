/*!
 * \file HDRIToPrefilteredSpecMap.comp.glsl
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
#include "../PBRCommon.inl.glsl"

// 1, 2, 3 are used for subgrp size
layout(constant_id = 4) const uint MIP_COUNT = 5;
layout(constant_id = 5) const uint SAMPLE_COUNT = 1024;

layout(push_constant) uniform Constants
{
    uint sourceSize;
} constants;

layout (set = 0, binding = 0, rgba16f) writeonly uniform imageCube outPrefilteredSpecMap[MIP_COUNT];

layout (set = 1, binding = 0) uniform sampler2D hdri;

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

vec3 calculateIrrad(vec3 normal, float roughness)
{
    // view, reflection and normal are all assumed to be same
    //vec3 reflectDir = normal;
    vec3 pt2View = normal;
    vec3 specularIrrad = vec3(0.0);

    float totalWeight = 0.0;
    for(uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        vec2 uniformSample = hammersley(i, SAMPLE_COUNT);
        vec3 halfVector = importanceSampleGGX(uniformSample, normal, roughness);
        vec3 pt2Light = normalize(2 * dot(pt2View, halfVector) * halfVector - pt2View);

        float nDotL = max(dot(normal, pt2Light), 0.0);
        float ndotH = max(dot(normal, halfVector), 0.0);
        float pt2vDotH = max(dot(pt2View, halfVector), 0.0);

        float ndf = ggxtrNDF(normal, halfVector, roughness);
        float pdf = (ndf * ndotH / (4.0 * pt2vDotH)) + 0.0001; 
        // Solid angle of 1 pixel across all cube faces
        float saTexel  = 4.0 * M_PI / (6.0 * SQR(constants.sourceSize));    
        // Slid angle of current sample
        float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

        float mipLevel = (roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel)); 
        specularIrrad += (textureLod(hdri, cartesianToSphericalUV(pt2Light), mipLevel).xyz * nDotL);
        //specularIrrad += (texture(hdri, cartesianToSphericalUV(pt2Light)).xyz * nDotL);
        totalWeight += nDotL;
    }
    
    return specularIrrad / totalWeight;
}

// We are rendering like usual world view but we are assigning x to z, y to x and z to y as in vulkan cube map z is fwd x is right and y is up
// While sampling we do the same 
// And also when rendering cubemap same has to assigned to each layers
void mainComp()
{
    for(uint mipLevel = 0; mipLevel < MIP_COUNT; ++mipLevel)
    {
        ivec2 outSize = imageSize(outPrefilteredSpecMap[mipLevel]);
        if(gl_GlobalInvocationID.x >= outSize.x || gl_GlobalInvocationID.y >= outSize.y)
        {
            break;
        }
        
        vec2 texelUvSize = 1 / vec2(outSize);
        // Sample at center of texel
        vec2 outUV = vec2(gl_GlobalInvocationID.xy) * texelUvSize;
        outUV += texelUvSize * 0.5;
        float roughness = mipLevel / float(MIP_COUNT - 1);

        vec3 speculareIrrad;
    // All X's in Z faces 4, 5
    // Face along +x
        vec3 direction = normalize(vec3(CUBE_COORDS[2].x, mix(CUBE_COORDS[2].yz, CUBE_COORDS[1].yz, outUV)));
        speculareIrrad = calculateIrrad(direction, roughness);
        imageStore(outPrefilteredSpecMap[mipLevel], ivec3(gl_GlobalInvocationID.xy, 4), vec4(speculareIrrad, 1.0));
    
    // Face along -x
        direction = normalize(vec3(CUBE_COORDS[4].x, mix(CUBE_COORDS[4].yz, CUBE_COORDS[7].yz, outUV)));
        speculareIrrad = calculateIrrad(direction, roughness);
        imageStore(outPrefilteredSpecMap[mipLevel], ivec3(gl_GlobalInvocationID.xy, 5), vec4(speculareIrrad, 1.0));
    
    // All Y's in X faces 0,1
    // Face along +y
        vec2 faceSpaceUV = mix(CUBE_COORDS[0].xz, CUBE_COORDS[5].xz, outUV);
        direction = normalize(vec3(faceSpaceUV.x, CUBE_COORDS[0].y, faceSpaceUV.y));
        speculareIrrad = calculateIrrad(direction, roughness);
        imageStore(outPrefilteredSpecMap[mipLevel], ivec3(gl_GlobalInvocationID.xy, 0), vec4(speculareIrrad, 1.0));
    
    // Face along -y
        faceSpaceUV = mix(CUBE_COORDS[6].xz, CUBE_COORDS[3].xz, outUV);
        direction = normalize(vec3(faceSpaceUV.x, CUBE_COORDS[6].y, faceSpaceUV.y));
        speculareIrrad = calculateIrrad(direction, roughness);
        imageStore(outPrefilteredSpecMap[mipLevel], ivec3(gl_GlobalInvocationID.xy, 1), vec4(speculareIrrad, 1.0));
        
    // All Z's in Y faces 2, 3
    // Face along +z, flipped uv because U goes from - Y to + Y and V goes from -X to +X
        direction = normalize(vec3(mix(CUBE_COORDS[6].xy, CUBE_COORDS[0].xy, outUV.yx), CUBE_COORDS[6].z));
        speculareIrrad = calculateIrrad(direction, roughness);
        imageStore(outPrefilteredSpecMap[mipLevel], ivec3(gl_GlobalInvocationID.xy, 2), vec4(speculareIrrad, 1.0));
    
    // Face along -z, flipped uv because U goes from - Y to + Y and V goes from +X to -X
        direction = normalize(vec3(mix(CUBE_COORDS[3].xy, CUBE_COORDS[5].xy, outUV.yx), CUBE_COORDS[3].z));
        speculareIrrad = calculateIrrad(direction, roughness);
        imageStore(outPrefilteredSpecMap[mipLevel], ivec3(gl_GlobalInvocationID.xy, 3), vec4(speculareIrrad, 1.0));    
    }
}