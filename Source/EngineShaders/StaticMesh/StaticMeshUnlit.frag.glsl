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
    colorAttachment0 = mix(texture(diffuseTexture, inTextureCoord), inVertexColor, pushConsts.vertexDiffuseColorBlend);
    colorAttachment1 = vec4(inWorldNormal, 1);
    colorAttachment2 = inWorldPosition.w / 5000.f;
}