#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../../Common/ComputeCommon.inl.glsl"
#include "../PBRCommon.inl.glsl"

layout(constant_id = 5) const uint SAMPLE_COUNT = 1024;

layout (set = 0, binding = 0, rg16f) uniform writeonly image2D outIntegratedBrdf;

//layout(push_constant) uniform Constants
//{
//    float time;
//    uint flags;
//} constants;

void mainComp()
{
    ivec2 outSize = imageSize(outIntegratedBrdf);
    vec2 texelUvSize = 1 / vec2(outSize);
    // Sample at center of texel
    vec2 outUV = vec2(gl_GlobalInvocationID.xy) * texelUvSize;
    outUV += texelUvSize * 0.5;

    vec2 outValues = integrateBRDF(outUV.x, outUV.y, SAMPLE_COUNT);
    imageStore(outIntegratedBrdf, ivec2(gl_GlobalInvocationID.xy), vec4(outValues, 0, 1));
}