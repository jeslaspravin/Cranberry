/*!
 * \file PBRLightsNoShadow.frag.glsl
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

#define INPUT 1
#include "PBRStageIO.inl.glsl"
#undef INPUT

layout(location = 0) out vec4 colorAttachment0;

#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/CommonDefines.inl.glsl"
#include "../Common/ToneMapping.inl.glsl"
#include "PBRCommon.inl.glsl"

layout(set = 2, binding = 0) uniform sampler2D ssUnlitColor;
layout(set = 2, binding = 1) uniform sampler2D ssNormal;
layout(set = 2, binding = 2) uniform sampler2D ssDepth;
layout(set = 2, binding = 3) uniform sampler2D ssARM;
layout(set = 2, binding = 4) uniform sampler2D ssColor;
layout(set = 2, binding = 5) uniform samplerCube envMap;
layout(set = 2, binding = 6) uniform samplerCube diffuseIrradMap;
layout(set = 2, binding = 7) uniform samplerCube specEnvMap;
layout(set = 2, binding = 8) uniform sampler2D brdfLUT;

struct SpotLight
{
    vec4 sptLightColor_lumen;
    vec4 sptPos_radius;// Position and Radius
    vec4 sptDirection;
    vec2 sptCone;// inner, outer cone
};

struct PointLight
{
    vec4 ptLightColor_lumen;
    vec4 ptPos_radius;// Position and Radius
};

struct DirectionalLight
{
    vec4 lightColor_lumen;
    vec3 direction;
};

layout(set = 3, binding = 0) uniform ArrayOfLight 
{
    uint count;// 0-3 bits Spotlight, 4-7 bits Point light, 8-11 bits directional light cascade count

    SpotLight spotLits[8];
    PointLight ptLits[8];
    DirectionalLight dirLit;
} lightArray;

layout(set = 3, binding = 1) uniform ColorCorrection 
{
    float exposure;
    float gamma;
} colorCorrection;


vec3 prefilteredReflection(vec3 reflectDir, float roughness)
{
    const float maxSpecEnvLod = textureQueryLevels(specEnvMap) - 1;
//    float lod = roughness * maxSpecEnvLod;
//    float lodf = floor(lod);
//    float lodc = ceil(lod);
//    vec3 a = textureLod(specEnvMap, reflectDir, lodf).xyz;
//    vec3 b = textureLod(specEnvMap, reflectDir, lodc).xyz;
//    return mix(a, b, lod - lodf);
    return textureLod(specEnvMap, reflectDir, roughness * maxSpecEnvLod).xyz;
}

#define SPOT_COUNT (lightArray.count & 0x0000000F)
#define POINT_COUNT ((lightArray.count & 0x000000F0) >> 4)
#define DIRECTIONAL ((lightArray.count & 0x00000F00) > 0)

// Lo(p,o)= (C * ambient * ao) + integrate over sphere(kd * (lambert diffuse / pi)+ ((NDF * BRDF * N-Mask) / (4 * (o | n) * (i | n))) * Li(p,i) * (n | i)
void mainFS()
{
    const float depth = texture(ssDepth, inTextureCoord).x;
    const vec3 arm = texture(ssARM, inTextureCoord).xyz; 
    const vec3 worldPos = getWorldPosition(vec4(inNdcCoord, depth, 1));
    const vec3 worldNormal = normalize((texture(ssNormal, inTextureCoord).xyz - vec3(0.5)) * 2);
    const vec4 unlitColor = texture(ssUnlitColor, inTextureCoord);

    const vec3 pt2ViewDir = normalize(viewPos() - worldPos);
    const float nDotPtV = max(dot(pt2ViewDir, worldNormal), 0.0);

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, unlitColor.xyz, arm.z);

    vec4 finalColor = texture(ssColor, inTextureCoord);
    vec4 prevResolveColor = finalColor;
    // Spot light
    uint lightCount = SPOT_COUNT;
    for(int i = 0; i < lightCount; ++i)
    {
        const vec3 pt2LightDir = normalize(lightArray.spotLits[i].sptPos_radius.xyz - worldPos);
        const float nDotL = max(dot(worldNormal, pt2LightDir), 0.0);
        const vec3 halfVec = normalize(pt2LightDir + pt2ViewDir);

        float attn = falloff(length(lightArray.spotLits[i].sptPos_radius.xyz - worldPos), lightArray.spotLits[i].sptPos_radius.w);

//        // Inverse square
//        const float d = length(lightArray.spotLits[i].sptPos_radius.xyz - worldPos);
//        float attn = 1.0/(d*d);
        attn *= 1.0 - clamp((lightArray.spotLits[i].sptCone.x 
                    - dot(-pt2LightDir, lightArray.spotLits[i].sptDirection.xyz))
                    / (lightArray.spotLits[i].sptCone.x - lightArray.spotLits[i].sptCone.y), 0.0, 1.0);

        vec3 inLight = lightArray.spotLits[i].sptLightColor_lumen.xyz * lightArray.spotLits[i].sptLightColor_lumen.w;
        inLight *= M_LUMEN2CAND(lightArray.spotLits[i].sptCone.y) * attn * nDotL;

        vec3 specTerm = fresnelSchlick(halfVec, pt2ViewDir, f0);
        
        vec3 diffuse = (vec3(1.0) - specTerm) * (1 - arm.z) * (unlitColor.xyz / M_PI);

        // F * D * G
        // Ks * F = SQR(specTerm) is made specTerm for artistic reasons
        vec3 specNum = specTerm * ggxtrNDF(worldNormal, halfVec, arm.y) * geoMaskSmith(worldNormal, pt2ViewDir, pt2LightDir, SQR(arm.y + 1) * 0.125);
        vec3 specular = specNum / max((4 * nDotPtV * nDotL), MIN_N_DOT_V);
        
        vec3 brdf = diffuse + specular;
        finalColor += vec4(brdf * inLight, 0); 
    }

    // Point light
    lightCount = POINT_COUNT;
    for(int i = 0; i < lightCount; ++i)
    {
        float attn = falloff(length(lightArray.ptLits[i].ptPos_radius.xyz - worldPos), lightArray.ptLits[i].ptPos_radius.w);

        const vec3 pt2LightDir = normalize(lightArray.ptLits[i].ptPos_radius.xyz - worldPos);
        const float nDotL = max(dot(worldNormal, pt2LightDir), 0.0);
        const vec3 halfVec = normalize(pt2LightDir + pt2ViewDir);
        
        vec3 inLight = lightArray.ptLits[i].ptLightColor_lumen.xyz * lightArray.ptLits[i].ptLightColor_lumen.w;
        inLight *= M_PT_LUMEN2CAND * attn * nDotL;

        vec3 specTerm = fresnelSchlick(halfVec, pt2ViewDir, f0);
        
        vec3 diffuse = (vec3(1.0) - specTerm) * (1 - arm.z) * (unlitColor.xyz / M_PI);

        // F * D * G
        // Ks * F = SQR(specTerm) is made specTerm for artistic reasons
        vec3 specNum = specTerm * ggxtrNDF(worldNormal, halfVec, arm.y) * geoMaskSmith(worldNormal, pt2ViewDir, pt2LightDir, SQR(arm.y + 1) * 0.125);
        vec3 specular = specNum / max((4 * nDotPtV * nDotL), MIN_N_DOT_V);
        
        vec3 brdf = diffuse + specular;
        finalColor += vec4(brdf * inLight, 0);
    }

    // Directional light
    if(DIRECTIONAL)
    {
        const vec3 pt2LightDir = -lightArray.dirLit.direction;
        const float nDotL = max(dot(worldNormal, pt2LightDir), 0.0);
        const vec3 halfVec = normalize(pt2LightDir + pt2ViewDir);
        
        vec3 inLight = lightArray.dirLit.lightColor_lumen.xyz * lightArray.dirLit.lightColor_lumen.w;
        inLight *= M_PT_LUMEN2CAND * nDotL;

        vec3 specTerm = fresnelSchlick(halfVec, pt2ViewDir, f0);
        
        vec3 diffuse = (vec3(1.0) - specTerm) * (1 - arm.z) * (unlitColor.xyz / M_PI);

        // F * D * G
        // Ks * F = SQR(specTerm) is made specTerm for artistic reasons
        vec3 specNum = specTerm * ggxtrNDF(worldNormal, halfVec, arm.y) * geoMaskSmith(worldNormal, pt2ViewDir, pt2LightDir, SQR(arm.y + 1) * 0.125);
        vec3 specular = specNum / max((4 * nDotPtV * nDotL), MIN_N_DOT_V);
        
        vec3 brdf = diffuse + specular;

        vec3 outColor = finalColor.xyz + (brdf * inLight);

        // Env map
        vec3 sampleDir = getWorldPosition(vec4(inNdcCoord, 1, 1)) - viewPos();
        vec3 envColor = texture(envMap, ENGINE_WORLD_TO_CUBE_DIR(sampleDir)).xyz;
        // Tonemap and Gamma correct
        envColor = Uncharted2Tonemap(envColor, colorCorrection.exposure);
        prevResolveColor = vec4(GAMMA_CORRECT(envColor, colorCorrection.gamma), 1);

        // Doing ambient light here
        vec3 ambSpecTerm = fresnelSchlickRoughness(worldNormal, pt2ViewDir, f0, arm.y);
        // Diffuse
        vec3 ambDiffuse = (1 - ambSpecTerm) * (1 - arm.z) * texture(diffuseIrradMap, ENGINE_WORLD_TO_CUBE_DIR(worldNormal)).xyz * unlitColor.xyz;
        // Specular
        vec3 reflectDir = reflect(-pt2ViewDir, worldNormal);
        vec3 prefilteredColor = prefilteredReflection(ENGINE_WORLD_TO_CUBE_DIR(reflectDir), arm.y);
        vec2 ambSpecBrdf = texture(brdfLUT, vec2(nDotPtV, arm.y)).xy; 
        vec3 ambSpec = prefilteredColor * (ambSpecTerm * ambSpecBrdf.x + ambSpecBrdf.y);
        outColor += (ambDiffuse + ambSpec) * arm.x;

        // Tonemap
        outColor = Uncharted2Tonemap(outColor, colorCorrection.exposure);
        // Gamma correction
        outColor = GAMMA_CORRECT(outColor, colorCorrection.gamma);

        finalColor = vec4(outColor, finalColor.w);
        //finalColor = vec4(vec2(nDotPtV, arm.y), 0.0 , 1.0);
    }

    colorAttachment0 = depth == 0? prevResolveColor : finalColor;
}