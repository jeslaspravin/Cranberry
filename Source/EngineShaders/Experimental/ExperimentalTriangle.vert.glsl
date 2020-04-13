#version 450

layout(location = 0) out vec3 vertexColor;

struct VertInfo
{
    vec4 position;
    vec3 color;
};

const VertInfo vertices[3]={
    { { 0.0f , 0.75f , 0.0f , 1.0f},{ 1.0f, 0.0f, 0.0f } },
    { { 0.75f , -0.75f , 0.0f , 1.0f},{ 0.0f, 1.0f, 0.0f } },
    { { -0.75f , -0.75f , 0.0f , 1.0f},{ 0.0f, 0.0f, 1.0f } }
};

void mainVS()
{
    gl_Position = vertices[gl_VertexIndex].position;
    vertexColor = vertices[gl_VertexIndex].color;
}
