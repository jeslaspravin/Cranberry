#version 450
#extension GL_GOOGLE_include_directive:enable

layout(triangles, invocations = 6) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) out vec3 outWorldPos;

layout(set = 3, binding = 0) uniform PointShadowDepthViews
{
    mat4 w2Clip[6];
} lightViews;

void mainGeo()
{
    gl_Layer = gl_InvocationID;
    for(int i = 0; i < 3; ++i)
    {
        outWorldPos = gl_in[i].gl_Position.xyz;
        gl_Position = lightViews.w2Clip[gl_InvocationID] * gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}