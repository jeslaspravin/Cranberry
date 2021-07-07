#version 450
#extension GL_GOOGLE_include_directive:enable

#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "SingleColorStageIO.inl.glsl"
#undef INPUT

#undef MULTIBUFFER

#include "SingleColorDescriptors.inl.glsl"

void mainFS()
{
    colorAttachment0 = meshData.meshColor;
    colorAttachment1 = vec4(0,0,1,1);
}