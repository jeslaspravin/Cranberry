#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "GoochModelStageIO.inl.glsl"
#undef INPUT

#undef STATE_MESH
#undef MULTIBUFFER

void mainFS()
{    
    colorAttachment0 = inColor;
    colorAttachment1 = vec4((inWorldNormal + 1) * 0.5, 1);
    colorAttachment2 = inWorldPosition.w;
}