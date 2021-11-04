#version 450
#extension GL_GOOGLE_include_directive:enable

layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 colorAttachment0;

void mainFS()
{
    colorAttachment0 = inColor;
}