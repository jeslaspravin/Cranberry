#version 450

layout(location = 0) in vec3 coordinate;

layout(location = 0) out vec2 outTextureCoord;

void mainVS()
{
    gl_Position = vec4(coordinate,1);
    outTextureCoord = (coordinate.xy + 1) * 0.5f;
}