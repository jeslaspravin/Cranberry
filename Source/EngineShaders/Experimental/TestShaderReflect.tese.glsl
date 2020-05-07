#version 450

layout(triangles , equal_spacing, cw) in;

layout(location = 0) in vec4 inPosition[];
layout(location = 1) in vec4 inColor[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in float inVal1[];
layout(location = 4) in int inVal2[];

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out float outVal1;
layout(location = 4) out int outVal2;

void mainTE()
{
    gl_Position = gl_in[0].gl_Position * gl_TessCoord.x + gl_in[1].gl_Position * gl_TessCoord.y + gl_in[2].gl_Position * gl_TessCoord.z;
    outPosition = inPosition[0] * gl_TessCoord.x + inPosition[1] * gl_TessCoord.y + inPosition[2] * gl_TessCoord.z;
    outColor = inColor[0] * gl_TessCoord.x + inColor[1] * gl_TessCoord.y + inColor[2] * gl_TessCoord.z;
    outNormal = inNormal[0] * gl_TessCoord.x + inNormal[1] * gl_TessCoord.y + inNormal[2] * gl_TessCoord.z;
    outVal1 = inVal1[0] * gl_TessCoord.x + inVal1[1] * gl_TessCoord.y + inVal1[2] * gl_TessCoord.z;
    outVal2 = int(inVal2[0] * gl_TessCoord.x + inVal2[1] * gl_TessCoord.y + inVal2[2] * gl_TessCoord.z);
}