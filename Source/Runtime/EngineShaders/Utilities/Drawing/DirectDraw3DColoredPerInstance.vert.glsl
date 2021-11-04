#version 450
#extension GL_GOOGLE_include_directive:enable

//
// Draws 3D geometries colored per instance, all vertices are in world space
//
#define SIMPLE3D 1
#include "../../Common/VertexInputs.inl.glsl"
#undef SIMPLE3D

#include "../../Common/ViewDescriptors.inl.glsl"

void mainVS()
{
    gl_Position = viewData.projection * viewData.invView * vec4(position, 1);
}