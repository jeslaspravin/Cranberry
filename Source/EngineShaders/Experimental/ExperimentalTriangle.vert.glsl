#version 450

layout(push_constant) uniform PushConstants
{
    float rotation;
} pushConstants;

layout(location = 0) out vec3 vertexColor;

struct VertInfo
{
    vec3 position;
    vec3 color;
};

const VertInfo vertices[3]={
    { { 0.0f , -0.75f , 0.0f },{ 1.0f, 0.0f, 0.0f } },
    { { 0.75f , 0.75f , 0.0f },{ 0.0f, 1.0f, 0.0f } },
    { { -0.75f , 0.75f , 0.0f },{ 0.0f, 0.0f, 1.0f } }
};

void mainVS()
{
    mat3 rotMat = {{1,0,0},{0,1,0},{0,0,1}};
    rotMat[0] = vec3(cos(pushConstants.rotation), sin(pushConstants.rotation),0);
    rotMat[1] = vec3(-rotMat[0].y,rotMat[0].x,0);
    gl_Position = vec4(rotMat * vertices[gl_VertexIndex].position,1.0f);
    vertexColor = vertices[gl_VertexIndex].color;
}
