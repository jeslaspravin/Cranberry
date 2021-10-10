#version 450
#extension GL_GOOGLE_include_directive:enable

#define SIMPLE2D 1
#include "../Common/VertexInputs.inl.glsl"
#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/VertexInstanceDescriptors.inl.glsl"

#define OUTPUT 1
#include "SingleColorStageIO.inl.glsl"
#undef OUTPUT

#undef SIMPLE2D

void mainVS()
{
    vec4 worldPos = instancesWrapper.instances[gl_InstanceIndex].model * vec4(position.xy, 0, 1);
    outWorldPosition = worldPos.xyz;
    outMaterialIdx = instancesWrapper.instances[gl_InstanceIndex].shaderUniqIdx;
    gl_Position = viewData.projection * viewData.invView * worldPos;
}