#version 450
#extension GL_GOOGLE_include_directive:enable

#define SIMPLE3D 1
#include "../Common/VertexInputs.inl.glsl"
#undef SIMPLE3D

layout(location = 0) out vec2 outTextureCoord;

void mainVS()
{
    gl_Position = vec4(position,1);
    outTextureCoord = (position.xy + 1) * 0.5f;
}