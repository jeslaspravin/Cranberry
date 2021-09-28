#version 450
#extension GL_GOOGLE_include_directive:enable

layout(triangles, invocations = 8) in;
layout(triangle_strip, max_vertices = 3) out; 

layout(set = 3, binding = 0) uniform DirectionalShadowCascadeViews
{
    mat4 cascadeW2Clip[8];
    uint cascadeCount;
} lightViews;

void mainGeo()
{
    if (gl_InvocationID >= lightViews.cascadeCount)
    {
        return;
    }

    for(int i = 0; i < 3; ++i)
    {
        gl_Position = lightViews.cascadeW2Clip[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}