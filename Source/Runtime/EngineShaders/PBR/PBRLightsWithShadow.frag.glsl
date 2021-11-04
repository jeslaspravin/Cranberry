#version 450
#extension GL_GOOGLE_include_directive:enable

#define INPUT 1
#include "PBRStageIO.inl.glsl"
#undef INPUT

#define USE_RANDOM_SAMPLES 1

layout(location = 0) out vec4 colorAttachment0;

#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/CommonDefines.inl.glsl"
#include "../Common/ToneMapping.inl.glsl"
#if USE_RANDOM_SAMPLES
#include "../Common/RandomFunctions.inl.glsl"
#endif
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

layout(set = 3, binding = 2) uniform ShadowData
{
    // World to clip
    mat4 sptLitsW2C[8];
    // Max 8 cascades
    mat4 dirLitCascadesW2C[8];
    // Far distance for each cascade
    float cascadeFarPlane[8];
    uint shadowFlags;
} shadowData;
layout(set = 3, binding = 3) uniform sampler2DArray directionalLightCascades;
layout(set = 3, binding = 4) uniform samplerCube pointShadowMaps[8];
layout(set = 3, binding = 5) uniform sampler2D spotLightShadowMaps[8];

layout(push_constant) uniform Constants
{
    uint debugDrawFlags;
} constants;

layout(constant_id = 1) const int PCF_KERNEL_SIZE = 3;
layout(constant_id = 2) const int POINT_PCF_SAMPLES = 4;
layout(constant_id = 3) const float POINT_PCF_KERNEL_EXTEND = 0.2;

#define SPOT_COUNT (lightArray.count & 0x0000000F)
#define POINT_COUNT ((lightArray.count & 0x000000F0) >> 4)
#define DIRECTIONAL_CASCADES_COUNT ((lightArray.count & 0x00000F00) >> 8)
#define DRAWING_BACKFACE ((shadowData.shadowFlags & 0x00000001) == 0x00000001)

// Debug draw 
#define DebugDisableEnvAmbient 1
#define DebugDisableDirectional 2
#define DebugDisableAmbNDir 3
#define DebugDisableShadows 4
#define DebugDrawCascade 5

const vec3 CASCADE_COLORS[8] = 
{ 
    vec3(0.0, 1, 0.75),
    vec3(0.14901960784313725, 0.792156862745098, 0.7019607843137254),
    vec3(0.7294117647058823, 0.9372549019607843, 0.8196078431372549),
    vec3(0.8235294117647058, 0.7254901960784313, 0.4745098039215686),
    vec3(0.9450980392156862, 0.6784313725490196, 0.6352941176470588),
    vec3(0.9882352941176471, 0.39215686274509803, 0.4470588235294118),
    vec3(0.8235294117647058, 0.21568627450980393, 0.22745098039215686),
    vec3(1.0, 0.0, 0.0)
};

vec3 debugCascadeColor(vec3 worldPos, uint cascadeCnt)
{
    // No need to abs here as all pixel in screen is in front of camera
    float pixelViewDistance = dot((worldPos - viewPos()), viewFwd());
    for(int i = 0; i < cascadeCnt; ++i)
    {
        if(shadowData.cascadeFarPlane[i] > pixelViewDistance)
        {
            return CASCADE_COLORS[i];
        }
    }    
    return vec3(1.0);
}

// Implementations

vec3 prefilteredReflection(vec3 reflectDir, float roughness)
{
    const float maxSpecEnvLod = textureQueryLevels(specEnvMap) - 1;
//	float lod = roughness * maxSpecEnvLod;
//	float lodf = floor(lod);
//	float lodc = ceil(lod);
//	vec3 a = textureLod(specEnvMap, reflectDir, lodf).xyz;
//	vec3 b = textureLod(specEnvMap, reflectDir, lodc).xyz;
//	return mix(a, b, lod - lodf);
    return textureLod(specEnvMap, reflectDir, roughness * maxSpecEnvLod).xyz;
}

float directionalLightShadow(vec3 worldPos, vec3 worldNormal, vec3 pt2LightDir, uint cascadeCnt)
{
    // No need to abs here as all pixel in screen is in front of camera
    float pixelViewDistance = dot((worldPos - viewPos()), viewFwd());
    int cascadeIdx = -1;
    float nearPlane = 0.0;
    for(int i = 0; i < cascadeCnt; ++i)
    {
        if(shadowData.cascadeFarPlane[i] > pixelViewDistance)
        {
            cascadeIdx = i;
            break;
        }
        nearPlane = shadowData.cascadeFarPlane[i];
    }    

    // We can ignore view frustum space clipping as we are sure everything will be inside frustum <- inside texture
    if(cascadeIdx < 0)
    {
        return 0.0f;
    }

    vec4 lighClipPos = shadowData.dirLitCascadesW2C[cascadeIdx] * vec4(worldPos, 1.0);
    vec3 projPos = lighClipPos.xyz / lighClipPos.w;
    // We are combining the offset in world to clip matrix
    // projPos = vec3((projPos.xy * 0.5 + 0.5), projPos.z);

    float bias = max(0.05 * (1.0 - dot(worldNormal, pt2LightDir)), 0.005);
    // Increasing bias as cascade plane gets far away
    // #TODO : Find better alternative
    float cascadeBias = sqrt(2.0 / shadowData.cascadeFarPlane[cascadeIdx]);
    for(int i = 0; i < cascadeIdx; ++i)
    {
        cascadeBias = sqrt(cascadeBias);
    }
    bias *= cascadeBias;
    //#HACK: -0.0005 to reduce slight light bleedings in edges
    bias = DRAWING_BACKFACE? -0.0005 : bias;
    float depthBiased = (projPos.z + bias);

    vec2 shadowTexelSize = 1.0 / vec2(textureSize(directionalLightCascades, 0));
    // PCF
    float pcfShadow = 0;
#if USE_RANDOM_SAMPLES
    for(int i = 0; i < SQR(PCF_KERNEL_SIZE); ++i)
    {
        // The position is rounded to the millimeter to avoid too much aliasing
        int index = int(16.0 * vec4Random(vec4(floor(worldPos * 10.0), i))) % 16;
        
        vec2 textureCoord = projPos.xy + (poissonDisk[index] * shadowTexelSize);
        float sampleDepth = texture(directionalLightCascades, vec3(textureCoord, cascadeIdx)).x;

        pcfShadow += (depthBiased < sampleDepth)? 1.0 : 0.0;
    }
#else // USE_RANDOM_SAMPLES
    int halfKernelSize = PCF_KERNEL_SIZE / 2;
    for(int x = -halfKernelSize; x <= halfKernelSize; ++x)
    {
        for(int y = -halfKernelSize; y <= halfKernelSize; ++y)
        {
            vec2 textureCoord = projPos.xy + (vec2(x, y) * shadowTexelSize);
            float sampleDepth = texture(directionalLightCascades, vec3(textureCoord, cascadeIdx)).x;

            pcfShadow += (depthBiased < sampleDepth)? 1.0 : 0.0;
        }
    }
#endif // USE_RANDOM_SAMPLES
    pcfShadow /= SQR(PCF_KERNEL_SIZE);

    return pcfShadow;
}


float pointLightShadow(vec3 worldPos, vec3 worldNormal, vec3 pt2LightDirNorm, int pointIdx)
{
    float p2LDist = length(lightArray.ptLits[pointIdx].ptPos_radius.xyz - worldPos);
    vec3 pt2LightDir = p2LDist * pt2LightDirNorm;

    // outside light's influence
    if(p2LDist > lightArray.ptLits[pointIdx].ptPos_radius.w)
    {
        return 0.0f;
    }

    float bias = max(0.05 * 
        (1.0 - dot(worldNormal, pt2LightDirNorm))
        , 0.005);
    // Reducing bias on distance is causing acne at far distance, right now constant bias with respect to distance is fine
    // bias *= sqrt(1 - (p2LDist / lightArray.ptLits[pointIdx].ptPos_radius.w));// reduce bias as distance increases
    bias = DRAWING_BACKFACE? 0.0 : bias;

    // PCF
    float pcfShadow = 0;
//#if USE_RANDOM_SAMPLES
//    for(int i = 0; i < (POINT_PCF_SAMPLES * POINT_PCF_SAMPLES * POINT_PCF_SAMPLES); ++i)
//    {
//        // The position is rounded to the millimeter to avoid too much aliasing
//        int index = int(16.0 * vec4Random(vec4(floor(worldPos * 10.0), i))) % 16;        
//        vec3 bitan   = normalize(cross(pt2LightDir, (abs(pt2LightDir.x) < 0.999 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 0.0, 1.0))));
//        vec3 tangent = cross(bitan, pt2LightDir);
//        vec3 textureCoord = tangent * poissonDisk[index].x * POINT_PCF_KERNEL_EXTEND 
//            + bitan * poissonDisk[index].y * POINT_PCF_KERNEL_EXTEND 
//            + pt2LightDir;
//        textureCoord = -textureCoord;
//
//        // Since 1.0 is near plane
//        float sampleDepth = 1.0 - texture(pointShadowMaps[pointIdx], ENGINE_WORLD_TO_CUBE_DIR(textureCoord)).x;
//        // + bias as now greater distance means further away
//        sampleDepth = (sampleDepth + bias) * lightArray.ptLits[pointIdx].ptPos_radius.w;
//        // sampleDepth = (sampleDepth * lightArray.ptLits[pointIdx].ptPos_radius.w) + bias;
//                
//        pcfShadow += (p2LDist > sampleDepth)? 1.0 : 0.0;
//    }
//#else // USE_RANDOM_SAMPLES
    int halfKernelSize = PCF_KERNEL_SIZE / 2;
    for(float x = -POINT_PCF_KERNEL_EXTEND; x < POINT_PCF_KERNEL_EXTEND; x += (2 * POINT_PCF_KERNEL_EXTEND / POINT_PCF_SAMPLES))
    {
        for(float y = -POINT_PCF_KERNEL_EXTEND; y < POINT_PCF_KERNEL_EXTEND; y += (2 * POINT_PCF_KERNEL_EXTEND / POINT_PCF_SAMPLES))
        {
            for(float z = -POINT_PCF_KERNEL_EXTEND; z < POINT_PCF_KERNEL_EXTEND; z += (2 * POINT_PCF_KERNEL_EXTEND / POINT_PCF_SAMPLES))
            {
                vec3 textureCoord = -pt2LightDir + vec3(x, y, z);
                // Since 1.0 is near plane
                float sampleDepth = 1.0 - texture(pointShadowMaps[pointIdx], ENGINE_WORLD_TO_CUBE_DIR(textureCoord)).x;
                // + bias as now greater distance means further away
                sampleDepth = (sampleDepth + bias) * lightArray.ptLits[pointIdx].ptPos_radius.w;
                // sampleDepth = (sampleDepth * lightArray.ptLits[pointIdx].ptPos_radius.w) + bias;
                
                pcfShadow += (p2LDist > sampleDepth)? 1.0 : 0.0;
            }
        }
    }
//#endif
    pcfShadow /= (POINT_PCF_SAMPLES * POINT_PCF_SAMPLES * POINT_PCF_SAMPLES);

    return pcfShadow;
}


float spotLightShadow(vec3 worldPos, vec3 worldNormal, vec3 pt2LightDir, int spotIdx)
{    
    vec4 lighClipPos = shadowData.sptLitsW2C[spotIdx] * vec4(worldPos, 1.0);
    vec3 projPos = lighClipPos.xyz / lighClipPos.w;
    // We are combining the offset in world to clip matrix
    // projPos = vec3((projPos.xy * 0.5 + 0.5), projPos.z);

    // We dont have to handle < 0.0 case as they are handled with light shading
    if(projPos.z > 1.0)
    {
        return 0.0;
    }

    float bias = max(0.005 * (1.0 - dot(worldNormal, pt2LightDir)), 0.0005);
    bias *= sqrt(projPos.z); // reduce bias as distance increases
    //#HACK: -0.000005 to reduce slight light bleedings in edges
    bias = DRAWING_BACKFACE? -0.000005 : bias;
    float depthBiased = (projPos.z + bias);

    vec2 shadowTexelSize = 1.0 / vec2(textureSize(spotLightShadowMaps[spotIdx], 0));
    // PCF
    float pcfShadow = 0;
#if USE_RANDOM_SAMPLES
    for(int i = 0; i < SQR(PCF_KERNEL_SIZE); ++i)
    {
        // The position is rounded to the millimeter to avoid too much aliasing
        int index = int(16.0 * vec4Random(vec4(floor(worldPos * 10.0), i))) % 16;
        
        vec2 textureCoord = projPos.xy + (poissonDisk[index] * shadowTexelSize);
        float sampleDepth = texture(spotLightShadowMaps[spotIdx], textureCoord).x;

        pcfShadow += (depthBiased < sampleDepth)? 1.0 : 0.0;
    }
#else // USE_RANDOM_SAMPLES
    int halfKernelSize = PCF_KERNEL_SIZE / 2;
    for(int x = -halfKernelSize; x <= halfKernelSize; ++x)
    {
        for(int y = -halfKernelSize; y <= halfKernelSize; ++y)
        {
            vec2 textureCoord = projPos.xy + (vec2(x, y) * shadowTexelSize);
            float sampleDepth = texture(spotLightShadowMaps[spotIdx], textureCoord).x;

            pcfShadow += (depthBiased < sampleDepth)? 1.0 : 0.0;
        }
    }
#endif
    pcfShadow /= SQR(PCF_KERNEL_SIZE);

    return pcfShadow;
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

    float shadowMultiplier = constants.debugDrawFlags == DebugDisableShadows? 0.0 : 1.0;

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
        float sptShadow = spotLightShadow(worldPos, worldNormal, pt2LightDir, i) * shadowMultiplier;

        finalColor += vec4(brdf * inLight * (1 - sptShadow), 0); 
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
        float ptShadow = pointLightShadow(worldPos, worldNormal, pt2LightDir, i) * shadowMultiplier;

        finalColor += vec4(brdf * inLight * (1 - ptShadow), 0);
    }

    // Directional light
    lightCount = DIRECTIONAL_CASCADES_COUNT;
    if(lightCount > 0)
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
        float directionalShadow = directionalLightShadow(worldPos, worldNormal, pt2LightDir, lightCount) * shadowMultiplier;
        vec3 directionalRad = (brdf * inLight * (1.0 - directionalShadow));
        float debugDirLight = ((constants.debugDrawFlags == DebugDisableDirectional) || (constants.debugDrawFlags == DebugDisableAmbNDir))
            ? 0.0 : 1.0;

        vec3 outColor = finalColor.xyz + (directionalRad * debugDirLight);

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
        // Spec + diff weighted by material's ao
        vec3 ambientRad = (ambDiffuse + ambSpec) * arm.x;
        float debugAmb = ((constants.debugDrawFlags == DebugDisableEnvAmbient) || (constants.debugDrawFlags == DebugDisableAmbNDir)) 
            ? 0.0 : 1.0;

        outColor += ambientRad * debugAmb;

        // Tonemap
        outColor = Uncharted2Tonemap(outColor, colorCorrection.exposure);
        // Gamma correction
        outColor = GAMMA_CORRECT(outColor, colorCorrection.gamma);

        finalColor = vec4(outColor, finalColor.w);
        //finalColor = vec4(vec2(nDotPtV, arm.y), 0.0 , 1.0);        

        if(constants.debugDrawFlags == DebugDrawCascade)
        {
            finalColor = vec4(debugCascadeColor(worldPos, lightCount), 1.0);
        }
    }
    colorAttachment0 = depth == 0? prevResolveColor : finalColor;
}