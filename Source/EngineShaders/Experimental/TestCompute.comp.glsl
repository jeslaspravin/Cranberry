#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../Common/ComputeCommon.inl.glsl"

layout (binding = 0, rgba8) uniform image2D resultImage;	

void mainComp()
{								
	vec4 res = vec4(
		gl_GlobalInvocationID.x/gl_WorkGroupSize.x, 
		gl_GlobalInvocationID.y/gl_WorkGroupSize.y, 
		0.0,
		1.0);

	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), res);
}