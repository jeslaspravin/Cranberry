#version 450
#extension GL_GOOGLE_include_directive:enable

#define SIMPLE3D 1
#include "../Common/VertexInputs.inl.glsl"
#undef SIMPLE3D
#define OUTPUT 1
#include "PBRStageIO.inl.glsl"
#undef OUTPUT

// Including here to use this decriptors set created view shader params to be usable with any other shader that uses view params
#include "../Common/ViewDescriptors.inl.glsl"

void mainVS()
{
    gl_Position = vec4(position,1);
    outTextureCoord = (position.xy + 1) * 0.5f;
    // Fliping y since Quad draw uses vulkan screen coord top left -1,-1 bottom right 1,1. But our view/projection y coordinate is from bottom(-1) to top(1)
    outNdcCoord = vec2(position.x, -position.y);
}