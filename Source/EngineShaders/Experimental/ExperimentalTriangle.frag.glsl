#version 450

layout(location = 0) in vec3 vertexColor;

layout(location = 0) out vec4 colorAttachment0;

void mainFS()
{
    colorAttachment0 = vec4(vertexColor,1);
}