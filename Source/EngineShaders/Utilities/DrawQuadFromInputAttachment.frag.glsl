#version 450

layout(location = 0) in vec2 inTextureCoord;

layout(location = 0) out vec4 colorAttachment0;

// layout(set = 0, binding = 2) uniform sampler textureSampler;
// layout(set = 0, binding = 3) uniform texture2D quadTexture;
layout(input_attachment_index = 0,set = 0, binding = 0) uniform subpassInput quadTexture;

void mainFS()
{
    //colorAttachment0 = texture(sampler2D(quadTexture,textureSampler),inTextureCoord);
    colorAttachment0 = subpassLoad(quadTexture);
}