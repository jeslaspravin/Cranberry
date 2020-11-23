#version 450

layout(location = 0) in vec2 inTextureCoord;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 colorAttachment0;

layout(set = 0, binding = 1) uniform sampler2D fontAtlas;

void mainFS()
{
    colorAttachment0 = inColor * texture(fontAtlas, inTextureCoord);
}