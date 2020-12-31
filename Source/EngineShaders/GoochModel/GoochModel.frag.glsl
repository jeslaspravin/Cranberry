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

struct  LightData
{
    vec4 warmOffsetAndPosX;
    vec4 coolOffsetAndPosY;
    vec4 highlightColorAndPosZ;
    vec4 lightColorAndRadius;
};

layout(set = 2, binding = 0) uniform ArrayOfLight 
{
    LightData lights[10];
    uint count;
} lightArray;

// Define Code
//        const vec3 lightDir = vec3(lightArray.lights[index].warmOffsetAndPosX.w, lightArray.lights[index].coolOffsetAndPosY.w, lightArray.lights[index].highlightColorAndPosZ.w) - worldPos;
//        float lightFalloff = smoothstep(0.0, 1.0, 1.0 - clamp(length(lightDir)/1000.0, 0.0, 1.0));
//        lightDir=normalize(lightDir);
//        const float lDn = dot(lightDir, worldNormal);                                                                                                                                     
//        const vec3 rLightDir = 2 * lDn * worldNormal - lightDir;                                                                                                                          
//// For perspective camera                                                                                                                                                           
//        const vec4 litColor = mix(vec4(lightArray.lights[index].warmOffsetAndPosX.xyz + 0.25f * unlitColor.xyz, unlitColor.w)                                                       
//            , vec4(lightArray.lights[index].highlightColorAndPosZ.xyz, unlitColor.w), clamp(100 * dot(rLightDir, normalize(viewPos() - worldPos)) - 97, 0.0, 1.0));                 
//// NOTE : This is correct in only orthographic camera                                                                                                                               
////      const vec4 litColor = mix(vec4(lightArray.lights[index].warmOffsetAndPosX.xyz + 0.25f * unlitColor.xyz, unlitColor.w)                                                       
////          , vec4(lightArray.lights[index].highlightColorAndPosZ.xyz, unlitColor.w), clamp(100 * dot(rLightDir, -viewFwd()) - 97, 0.0, 1.0));                                      
//                                                                                                                                                                                    
//        const vec4 coolColor = vec4(lightArray.lights[index].coolOffsetAndPosY.xyz + 0.25f * unlitColor.xyz, unlitColor.w);                                                         
//        finalColor += lightFalloff * (0.5 * lightCommon.invLightsCount * coolColor + clamp(lDn, 0.0, 1.0) * vec4(lightArray.lights[index].lightColorAndRadius.xyz, unlitColor.w) * litColor)
//
#define LIGHTCALC_LOOP_BODY(index) lightDir = vec3(lightArray.lights[index].warmOffsetAndPosX.w, lightArray.lights[index].coolOffsetAndPosY.w, lightArray.lights[index].highlightColorAndPosZ.w) - worldPos;lightFalloff = smoothstep(0.0, 1.0, clamp(1.0 - length(lightDir)/1000.0, 0.0, 1.0));lightDir=normalize(lightDir);lDn = dot(lightDir, worldNormal);rLightDir = 2 * lDn * worldNormal - lightDir;litColor = mix(vec4(lightArray.lights[index].warmOffsetAndPosX.xyz + 0.25f * unlitColor.xyz, unlitColor.w), vec4(lightArray.lights[index].highlightColorAndPosZ.xyz, unlitColor.w), clamp(100 * dot(rLightDir, normalize(viewPos() - worldPos)) - 97, 0.0, 1.0));coolColor = vec4(lightArray.lights[index].coolOffsetAndPosY.xyz + 0.25f * unlitColor.xyz, unlitColor.w);finalColor += lightFalloff * (0.5 * lightCommon.invLightsCount * coolColor + clamp(lDn, 0.0, 1.0) * vec4(lightArray.lights[index].lightColorAndRadius.xyz, unlitColor.w) * litColor)

void mainFS()
{
    const float depth = texture(ssDepth, inTextureCoord).x;
    const vec3 worldPos = getWorldPosition(vec4(inNdcCoord, depth, 1));
    const vec3 worldNormal = texture(ssNormal, inTextureCoord).xyz;
    const vec4 unlitColor = texture(ssUnlitColor, inTextureCoord);

    vec4 finalColor = texture(ssColor, inTextureCoord);
    if(lightArray.count == 10)
    {
        vec3 lightDir;
        float lightFalloff;
        float lDn;
        vec3 rLightDir;
        vec4 litColor;
        vec4 coolColor;
        
        LIGHTCALC_LOOP_BODY(0);
        LIGHTCALC_LOOP_BODY(1);
        LIGHTCALC_LOOP_BODY(2);
        LIGHTCALC_LOOP_BODY(3);
        LIGHTCALC_LOOP_BODY(4);
        LIGHTCALC_LOOP_BODY(5);
        LIGHTCALC_LOOP_BODY(6);
        LIGHTCALC_LOOP_BODY(7);
        LIGHTCALC_LOOP_BODY(8);
        LIGHTCALC_LOOP_BODY(9);
    }
    else
    {
        vec3 lightDir;
        float lightFalloff;
        float lDn;
        vec3 rLightDir;
        vec4 litColor;
        vec4 coolColor;
        for(int i = 0; i < lightArray.count; i++)
        {
            LIGHTCALC_LOOP_BODY(i);
        }
    }
    colorAttachment0 = mix( texture(ssColor, inTextureCoord), finalColor, ceil(depth));
}