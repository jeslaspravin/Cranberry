#version 450

layout(vertices = 3) out;

layout(push_constant) uniform PushConstants
{
    layout(offset = 4) int tessellationCoeff;
} pushConstants;

layout(location = 0) in vec4 inPosition[];
layout(location = 1) in vec4 inColor[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in float inVal1[];
layout(location = 4) in int inVal2[];

layout(location = 0) out vec4 outPosition[];
layout(location = 1) out vec4 outColor[];
layout(location = 2) out vec3 outNormal[];
layout(location = 3) out float outVal1[];
layout(location = 4) out int outVal2[];

void mainTC()
{
    if(gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = 2 * pushConstants.tessellationCoeff;
        gl_TessLevelInner[1] = 1;
        gl_TessLevelOuter[0] = 2 * pushConstants.tessellationCoeff;
        gl_TessLevelOuter[1] = 4 * pushConstants.tessellationCoeff;
        gl_TessLevelOuter[2] = 3 * pushConstants.tessellationCoeff;
        gl_TessLevelOuter[3] = 1;
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outPosition[gl_InvocationID] = inPosition[gl_InvocationID];
    outColor[gl_InvocationID] = inColor[gl_InvocationID];
    outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    outVal1[gl_InvocationID] = inVal1[gl_InvocationID];
    outVal2[gl_InvocationID] = inVal2[gl_InvocationID];
}