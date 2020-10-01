#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#include "../Common/VertexInputs.inl.glsl"
#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/VertexInstanceDescriptors.inl.glsl"

#define OUTPUT 1
#include "DefaultStageIO.inl.glsl"
#undef OUTPUT

#undef STATIC_MESH

void mainVS()
{
    vec4 worldPos = instanceData.model * vec4(position.xyz, 1);
    vec4 viewPos = viewData.invView * worldPos;
    float f = - viewData.projection[2][3] / viewData.projection[2][2];
    float n = viewData.projection[2][3] / (1 - viewData.projection[2][2]);
    outWorldPosition = vec4(worldPos.xyz, 1 - viewPos.z/(f - n));
//    outWorldPosition = vec4(worldPos.xyz, viewPos.z);
    gl_Position = viewData.projection * viewPos;
    outLocalPosition = position.xyz;
    outWorldNormal = (transpose(instanceData.invModel) * vec4(normal.xyz, 1)).xyz;
    outLocalNormal = normal.xyz;
    outVertexColor = vertexColor;
    outTextureCoord = vec2(position.w, normal.w);
}