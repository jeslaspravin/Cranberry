#version 450
#extension GL_GOOGLE_include_directive:enable

#define INPUT 1
#include "GoochModelStageIO.inl.glsl"
#undef INPUT

layout(location = 0) out vec4 colorAttachment0;

#include "../Common/ViewDescriptors.inl.glsl"

layout(set = 0, binding = 1) uniform LightCommon 
{
    uint lightsCount;
    float invLightsCount;
} lightCommon;

layout(set = 1, binding = 0) uniform sampler2D ssUnlitColor;
layout(set = 1, binding = 1) uniform sampler2D ssNormal;
layout(set = 1, binding = 2) uniform sampler2D ssDepth;
layout(set = 1, binding = 3) uniform sampler2D ssColor;

layout(set = 2, binding = 0) uniform LightData
{
    vec4 warmOffsetAndPosX;
    vec4 coolOffsetAndPosY;
    vec4 highlightColorAndPosZ;
    vec4 lightColorAndRadius;
} light;

void mainFS()
{
    const float depth = texture(ssDepth, inTextureCoord).x;
    const vec3 worldPos = getWorldPosition(vec4(inNdcCoord, depth, 1));
    const vec3 worldNormal = texture(ssNormal, inTextureCoord).xyz;
    const vec3 lightDir = normalize(vec3(light.warmOffsetAndPosX.w, light.coolOffsetAndPosY.w, light.highlightColorAndPosZ.w) - worldPos);
    const float lDn = dot(lightDir, worldNormal);
    const vec3 rLightDir = 2 * lDn * worldNormal - lightDir;

    const vec4 unlitColor = texture(ssUnlitColor, inTextureCoord);
// For perspective camera
    const vec4 litColor = mix(vec4(light.warmOffsetAndPosX.xyz + 0.25f * unlitColor.xyz, unlitColor.w)
        , vec4(light.highlightColorAndPosZ.xyz, unlitColor.w), clamp(100 * dot(rLightDir, normalize(viewPos() - worldPos)) - 97, 0.0, 1.0));
// NOTE : This is correct in only orthographic camera
//    const vec4 litColor = mix(vec4(light.warmOffsetAndPosX.xyz + 0.25f * unlitColor.xyz, unlitColor.w)
//        , vec4(light.highlightColorAndPosZ.xyz, unlitColor.w), clamp(100 * dot(rLightDir, -viewFwd()) - 97, 0.0, 1.0));
    const vec4 coolColor = vec4(light.coolOffsetAndPosY.xyz + 0.25f * unlitColor.xyz, unlitColor.w);

    colorAttachment0 = mix( texture(ssColor, inTextureCoord), 0.5 * lightCommon.invLightsCount * coolColor + texture(ssColor, inTextureCoord)
        + clamp(lDn, 0.0, 1.0) * vec4(light.lightColorAndRadius.xyz, unlitColor.w) * litColor, ceil(depth));
}