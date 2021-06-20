#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../Common/ComputeCommon.inl.glsl"

layout (set = 0, binding = 0, rgba8) uniform image2D resultImage;	

layout(push_constant) uniform Constants
{
	float time;
	uint flags;
} constants;

void mainComp()
{								
//	vec4 res = vec4(
//		fract(float(gl_GlobalInvocationID.x)/gl_WorkGroupSize.x), 
//		fract(float(gl_GlobalInvocationID.y)/gl_WorkGroupSize.y), 
//		fract((gl_GlobalInvocationID.x + ((constants.flags & 0x00000001) > 0? constants.time : 0))/gl_WorkGroupSize.x)
//		* fract((gl_GlobalInvocationID.y + ((constants.flags & 0x00000010) > 0? constants.time : 0))/gl_WorkGroupSize.y),
//		1.0);
	vec4 res = vec4(
			fract((gl_GlobalInvocationID.x - ((constants.flags & 0x00000001) > 0? constants.time * 32: 0))/gl_WorkGroupSize.x), 
			fract((gl_GlobalInvocationID.y - ((constants.flags & 0x00000010) > 0? constants.time * 32: 0))/gl_WorkGroupSize.y), 
			0.0,
			1.0);

	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), res);
}