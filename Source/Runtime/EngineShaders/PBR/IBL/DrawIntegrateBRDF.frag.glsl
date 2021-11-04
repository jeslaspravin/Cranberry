#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../PBRCommon.inl.glsl"

layout(constant_id = 5) const uint SAMPLE_COUNT = 1024;

layout(location = 0) in vec2 inTextureCoord;

layout(location = 0) out vec4 colorAttachment0;

void mainFS()
{
	vec2 outValues = integrateBRDF(inTextureCoord.x, inTextureCoord.y, SAMPLE_COUNT);
    colorAttachment0 = vec4(outValues, 0.0, 1.0);
}