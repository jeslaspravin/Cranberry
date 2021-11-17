#version 450
#extension GL_GOOGLE_include_directive:enable
#extension GL_EXT_nonuniform_qualifier:enable

#include "../Common/ComputeCommon.inl.glsl"

layout (set = 0, binding = 0, rgba8) writeonly uniform image2D resultImage;    
layout (set = 0, binding = 1) uniform sampler2D srcImages[];    

struct AOS
{
    vec4 a;
    vec2 b;
    vec2 c[4];
};

layout (set = 0, binding = 2) readonly buffer TestAOS
{
    vec4 test1;
    AOS data[];
} inData;

layout(push_constant) uniform Constants
{
    float time;
    uint srcIndex;
    uint flags;
} constants;

void mainComp()
{                                
//    vec4 res = vec4(
//        fract(float(gl_GlobalInvocationID.x)/gl_WorkGroupSize.x), 
//        fract(float(gl_GlobalInvocationID.y)/gl_WorkGroupSize.y), 
//        fract((gl_GlobalInvocationID.x + ((constants.flags & 0x00000001) > 0? constants.time : 0))/gl_WorkGroupSize.x)
//        * fract((gl_GlobalInvocationID.y + ((constants.flags & 0x00000010) > 0? constants.time : 0))/gl_WorkGroupSize.y),
//        1.0);
//    vec4 res = vec4(
//            fract((gl_GlobalInvocationID.x - ((constants.flags & 0x00000001) > 0? constants.time * 32: 0))/gl_WorkGroupSize.x), 
//            fract((gl_GlobalInvocationID.y - ((constants.flags & 0x00000010) > 0? constants.time * 32: 0))/gl_WorkGroupSize.y), 
//            0.0,
//            1.0);
    const vec2 resultTexelSize = 1.0 / vec2(imageSize(resultImage));
    vec2 texSampleCoord = gl_GlobalInvocationID.xy * resultTexelSize + resultTexelSize * 0.5;
    imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(texture(srcImages[constants.srcIndex], texSampleCoord).xyz, 1.0));
}