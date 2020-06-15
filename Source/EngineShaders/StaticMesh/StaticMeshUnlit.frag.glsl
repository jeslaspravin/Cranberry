#version 450
#extension GL_GOOGLE_include_directive:enable

#include "StaticMeshUnlitInput.h"

layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D normalTexture; 

layout(location = 0) out vec4 colorAttachment0;// Color
layout(location = 1) out vec4 colorAttachment1;// Normal
layout(location = 2) out float colorAttachment2;// Depth

layout(push_constant) uniform PustConstants
{
    float vertexDiffuseColorBlend;
} pushConsts;

void mainFS()
{
    colorAttachment0 = vec4(mix(texture(diffuseTexture, inTextureCoord).xyz
        , vec3(fract(inWorldPosition.x/10.f),fract(inWorldPosition.y/10.f),fract(inWorldPosition.z/10.f)), pushConsts.vertexDiffuseColorBlend), 1);
    colorAttachment1 = vec4((inWorldNormal + 1) * 0.5, 1);
    colorAttachment2 = inWorldPosition.w;
}