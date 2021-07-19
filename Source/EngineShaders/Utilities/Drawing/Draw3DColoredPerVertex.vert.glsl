#version 450
#extension GL_GOOGLE_include_directive:enable

//
// Draws 3D geometries colored per vertex and transformed by mvp
//
#define SIMPLE3D_COLOR 1
#include "../../Common/VertexInputs.inl.glsl"
#include "../../Common/VertexInstanceDescriptors.inl.glsl"
#undef SIMPLE3D_COLOR

#include "../../Common/ViewDescriptors.inl.glsl"

layout(push_constant) uniform Constants
{
    float ptSize;
} constants;

layout(location=0) out vec4 outColor;

void mainVS()
{
    vec4 worldPos =  instanceData.model * vec4(position, 1);
    gl_Position = viewData.projection * viewData.invView * worldPos;
    gl_PointSize = constants.ptSize;
    outColor = color;
}