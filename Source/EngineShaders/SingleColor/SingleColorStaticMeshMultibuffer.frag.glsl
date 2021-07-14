#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "SingleColorStageIO.inl.glsl"
#undef INPUT

#undef STATE_MESH
#undef MULTIBUFFER

#include "SingleColorDescriptors.inl.glsl"

void mainFS()
{    
    colorAttachment0 = meshData.meshColor;
    colorAttachment1 = vec4((normalize(inWorldNormal) * 0.5) + 0.5, 1);
    colorAttachment2 = vec4(1, meshData.roughness, meshData.metallic, 1);
}