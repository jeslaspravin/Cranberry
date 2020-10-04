#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "GoochModelStageIO.inl.glsl"
#undef INPUT

#undef STATE_MESH
#undef MULTIBUFFER

layout(set = 2, binding = 0) uniform SurfaceData
{
    layout(offset = 0) vec3 lightPos;
    layout(offset = 16) vec4 highlightColor;
    layout(offset = 32) vec4 surfaceColor; 
} surfaceData;

void mainFS()
{    
    vec3 worldNormal = normalize(inWorldNormal);
    vec3 lightDir = normalize(surfaceData.lightPos - inWorldPosition.xyz);
    float colorCoeff =  (dot(worldNormal, lightDir) + 1) * 0.5;    
    float highlightCoeff = clamp((100 * dot((2 * dot(worldNormal, lightDir) * worldNormal - lightDir), -inViewFwd) - 97), 0.0, 1.0);
    //float highlightCoeff = clamp(dot((2 * dot(worldNormal, lightDir) * worldNormal - lightDir), -inViewFwd), 0.0, 1.0);

    colorAttachment0 = highlightCoeff * surfaceData.highlightColor + (1 - highlightCoeff) 
        * (colorCoeff * vec4(vec3(0.3, 0.3, 0.0) + 0.25 * surfaceData.surfaceColor.xyz, surfaceData.surfaceColor.w) 
        + (1 - colorCoeff) * vec4(vec3(0, 0, 0.55) + 0.25 * surfaceData.surfaceColor.xyz, surfaceData.surfaceColor.w));
    colorAttachment1 = vec4((worldNormal + 1) * 0.5, 1);
    colorAttachment2 = inWorldPosition.w;
}