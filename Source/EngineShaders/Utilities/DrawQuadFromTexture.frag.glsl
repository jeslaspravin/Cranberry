#version 450

layout(location = 0) in vec2 inTextureCoord;

layout(location = 0) out vec4 colorAttachment0;

layout(set = 0, binding = 0) uniform sampler2D quadTexture;

void mainFS()
{
    colorAttachment0 = texture(quadTexture, inTextureCoord);
}