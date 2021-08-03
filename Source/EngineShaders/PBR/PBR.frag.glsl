#version 450
#extension GL_GOOGLE_include_directive:enable

#define INPUT 1
#include "PBRStageIO.inl.glsl"
#undef INPUT

layout(location = 0) out vec4 colorAttachment0;

#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/CommonDefines.inl.glsl"
#include "../Common/ToneMapping.inl.glsl"

layout(set = 1, binding = 1) uniform sampler2D ssUnlitColor;
layout(set = 1, binding = 2) uniform sampler2D ssNormal;
layout(set = 1, binding = 3) uniform sampler2D ssDepth;
layout(set = 1, binding = 4) uniform sampler2D ssARM;
layout(set = 1, binding = 5) uniform sampler2D ssColor;
layout(set = 1, binding = 6) uniform samplerCube envMap;
layout(set = 1, binding = 7) uniform samplerCube diffuseIrradMap;

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

layout(set = 2, binding = 0) uniform ArrayOfLight 
{
    uint count;// 0-2 bits Spotlight, 3-5 bits Point light, 6th bit directional light

    SpotLight spotLits[8];
    PointLight ptLits[8];
    DirectionalLight dirLit;
} lightArray;

layout(set = 2, binding = 1) uniform ColorCorrection 
{
    float exposure;
    float gamma;
} colorCorrection;



#define SPOT_COUNT (lightArray.count & 0x0000000F)
#define POINT_COUNT ((lightArray.count & 0x000000F0) >> 4)
#define DIRECTIONAL ((lightArray.count & 0x00000100) > 0)

float ggxtrNDF(vec3 normal, vec3 halfVector, float roughness)
{
    float alpha = SQR(roughness);
    float alpha2 = SQR(alpha);
    float temp = max(dot(normal, halfVector), 0);
    temp = (SQR(temp) * (alpha2 - 1)) + 1;
    return alpha2 / (M_PI * SQR(temp));
}

// For lights k = ((rough + 1) ^ 2) / 8
// IBL k = (rough ^ 2)/2
float geoMaskSchlickGGX(vec3 normal, vec3 maskedDir, float k)
{
    float nDotD = max(dot(normal, maskedDir), 0.0);
    return nDotD/(nDotD * (1 - k) + k);
}

float geoMaskSmith(vec3 normal, vec3 pt2ViewDir, vec3 pt2LightDir, float k)
{
    return geoMaskSchlickGGX(normal, pt2ViewDir, k) * geoMaskSchlickGGX(normal, pt2LightDir, k);
}

vec3 fresnelSchlick(vec3 halfVec, vec3 viewDir, vec3 f0)
{
    return f0 + (1 - f0) * pow((1 - max(dot(halfVec, viewDir), 0.0)), 5);
}
vec3 fresnelSchlickRoughness(vec3 halfVec, vec3 viewDir, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1 - roughness), f0) - f0) * pow((1 - max(dot(halfVec, viewDir), 0.0)), 5);
}

// Epic games fall off
float falloff(float falloffDistance, float maxRadius ) 
{
	float falloff = 0;
	float distOverRadius = falloffDistance / maxRadius;
	float distOverRadius4 = SQR(SQR(distOverRadius));
	falloff = clamp(1.0 - distOverRadius4, 0.0, 1.0);
    falloff = SQR(falloff);
// Scaling to meters
	falloff /= SQR(falloffDistance * M_CM2M) + 1.0;
	return falloff;
}

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
    for(int i = 0; i < SPOT_COUNT; ++i)
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
        vec3 specNum = specTerm * ggxtrNDF(worldNormal, halfVec, arm.y) * geoMaskSmith(worldNormal, pt2ViewDir, pt2LightDir, SQR(arm.y + 1) * 0.125);
        vec3 specular = specNum / max((4 * nDotPtV * nDotL), 0.01);
        
        vec3 brdf = diffuse + specular;
        finalColor += vec4(brdf * inLight, 0); 
    }

    // Point light
    for(int i = 0; i < POINT_COUNT; ++i)
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
        vec3 specNum = specTerm * ggxtrNDF(worldNormal, halfVec, arm.y) * geoMaskSmith(worldNormal, pt2ViewDir, pt2LightDir, SQR(arm.y + 1) * 0.125);
        vec3 specular = specNum / max((4 * nDotPtV * nDotL), 0.01);
        
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
        vec3 specNum = specTerm * ggxtrNDF(worldNormal, halfVec, arm.y) * geoMaskSmith(worldNormal, pt2ViewDir, pt2LightDir, SQR(arm.y + 1) * 0.125);
        vec3 specular = specNum / max((4 * nDotPtV * nDotL), 0.01);
        
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
        vec3 ambDiffuse = (1 - ambSpecTerm) * texture(diffuseIrradMap, ENGINE_WORLD_TO_CUBE_DIR(worldNormal)).xyz * unlitColor.xyz;
        // outColor += (ambDiffuse + (ambSpecTerm * lightArray.dirLit.lightColor_lumen.xyz * 0.04)) * arm.x;
        outColor += ambDiffuse * arm.x;

        // Tonemap
        outColor = Uncharted2Tonemap(outColor, colorCorrection.exposure);
        // Gamma correction
        outColor = GAMMA_CORRECT(outColor, colorCorrection.gamma);

        finalColor = vec4(outColor, finalColor.w);
    }

    colorAttachment0 = depth == 0? prevResolveColor : finalColor;
}