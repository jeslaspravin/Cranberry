#version 450

layout(location = 0) in vec2 inTextureCoord;

layout(location = 0) out vec4 colorAttachment0;

layout(set = 0, binding = 0) uniform ClearInfo
{
    vec4 clearColor;
} clearInfo;

void mainFS()
{
    colorAttachment0 = clearInfo.clearColor;
}