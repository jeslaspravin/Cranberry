#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "DefaultStageIO.inl.glsl"
#undef INPUT

#undef STATE_MESH
#undef MULTIBUFFER

void mainFS()
{    
    colorAttachment0 = vec4(fract(inTextureCoord.x * 10.f),fract(inTextureCoord.y * 10.f), 0, 1);
    colorAttachment1 = vec4((normalize(inWorldNormal) * 0.5) + 0.5, 1);
    colorAttachment2 = vec4(0, 0, 0, 1);
}