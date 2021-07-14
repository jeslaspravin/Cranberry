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
    gl_Position = viewData.projection * viewData.invView * worldPos;
    outWorldPosition = worldPos.xyz;
    outLocalPosition = position.xyz;
    outWorldNormal = (transpose(instanceData.invModel) * vec4(normal.xyz, 0)).xyz;
    outLocalNormal = normal.xyz;
    outVertexColor = vertexColor;
    outTextureCoord = vec2(position.w, normal.w);
}