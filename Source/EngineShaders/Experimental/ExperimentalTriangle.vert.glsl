#version 450

layout(push_constant) uniform PushConstants
{
    float rotation;
} pushConstants;

layout(set = 0,binding = 0) uniform samplerBuffer vertsBuffer;

layout(location = 0) out vec3 vertexColor;

struct VertInfo
{
    vec3 position;
    vec3 color;
};

const vec3 colors[3]={
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f }
};

void mainVS()
{
    mat2 rotMat = {{1,0},{0,1}};
    rotMat[0] = vec2(cos(pushConstants.rotation), sin(pushConstants.rotation));
    rotMat[1] = vec2(-rotMat[0].y,rotMat[0].x);
    gl_Position = vec4(rotMat * texelFetch(vertsBuffer,gl_VertexIndex).xy, 0.0f, 1.0f);
    vertexColor = colors[gl_VertexIndex];
}
