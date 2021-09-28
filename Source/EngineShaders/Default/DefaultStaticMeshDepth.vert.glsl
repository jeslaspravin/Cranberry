#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#include "../Common/VertexInputs.inl.glsl"
#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/VertexInstanceDescriptors.inl.glsl"



#undef STATIC_MESH

void mainVS()
{
    gl_Position = viewData.projection * viewData.invView * instanceData.model * vec4(position.xyz, 1);
}