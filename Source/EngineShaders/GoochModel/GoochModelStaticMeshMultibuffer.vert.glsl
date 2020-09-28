#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#include "../Common/VertexInputs.inl.glsl"
#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/VertexInstanceDescriptors.inl.glsl"

#define OUTPUT 1
#include "GoochModelStageIO.inl.glsl"
#undef OUTPUT

#undef STATIC_MESH

layout(set = 2, binding = 0) uniform SurfaceData
{
    vec3 lightPos;
    vec4 highlightColor;
    vec4 surfaceColor; 
} surfaceData;

void mainVS()
{
    vec4 worldPos = instanceData.model * vec4(position.xyz, 1);
    vec4 clipPos = viewData.projection * viewData.invView * worldPos;
    outWorldPosition = vec4(worldPos.xyz, clipPos.z/clipPos.w);
    gl_Position = clipPos;
    outWorldNormal = (transpose(instanceData.invModel) * vec4(normal.xyz, 1)).xyz;

    vec3 lightDir = normalize(surfaceData.lightPos - worldPos.xyz);
    float colorCoeff =  (dot(outWorldNormal, lightDir) + 1) * 0.5;
    float highlightCoeff = clamp(100 * dot((2 * dot(outWorldNormal, lightDir) * outWorldNormal - lightDir), viewData.view[2].xyz) - 97, 0.0, 1.0);
    outColor = highlightCoeff * surfaceData.highlightColor + (1 - highlightCoeff) * (
        colorCoeff * vec4(vec3(0.3, 0.3, 0.0) + 0.25 * surfaceData.surfaceColor.xyz, surfaceData.surfaceColor.w) 
        + (1 - colorCoeff) * vec4(vec3(0, 0, 0.55) + 0.25 * surfaceData.surfaceColor.xyz, surfaceData.surfaceColor.w));
}