#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../../Common/CommonDefines.inl.glsl"

layout(location = 0) out vec4 colorAttachment0;

layout(push_constant) uniform Constants
{
    uint color;
} constants;

void mainFS()
{
    colorAttachment0 = UINT_RGBA_2_COLOR(constants.color);
}