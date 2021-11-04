#version 450

layout(location = 0) in vec2 inTextureCoord;

layout(location = 0) out vec4 colorAttachment0;

layout(input_attachment_index = 0,set = 0, binding = 0) uniform subpassInput quadTexture;

void mainFS()
{
    colorAttachment0 = subpassLoad(quadTexture);
}