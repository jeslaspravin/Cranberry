#version 450
#extension GL_GOOGLE_include_directive:enable

#include "StaticMeshVertex.h"
#include "StaticMeshUnlitOutput.h"

layout(set = 0, binding = 0) uniform ViewData
{
    mat4 view;
    mat4 invView;
    mat4 projection;
    mat4 invProjection;
} viewData;

layout(set = 1, binding = 0) uniform InstanceData
{
    mat4 model;
    mat4 invModel;
} instanceData;

void mainVS()
{
    vec4 worldPos = instanceData.model * vec4(position.xyz, 1);
    vec4 clipPos = viewData.projection * viewData.view * worldPos;
    outWorldPosition = vec4(worldPos.xyz, clipPos.w);
    gl_Position = clipPos;
    outLocalPosition = position.xyz;
    outWorldNormal = (transpose(instanceData.invModel) * vec4(normal.xyz, 1)).xyz;
    outLocalNormal = normal.xyz;
    outVertexColor = vertexColor;
    outTextureCoord = vec2(position.w, normal.w);
}
