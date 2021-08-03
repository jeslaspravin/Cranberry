#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../../Common/ComputeCommon.inl.glsl"
#include "../../Common/CommonDefines.inl.glsl"
#include "../../Common/CommonFunctions.inl.glsl"

layout (set = 0, binding = 0, rgba32f) uniform imageCube outDiffuseIrradiance;

layout (set = 1, binding = 0) uniform samplerCube envMap;

//layout(push_constant) uniform Constants
//{
//	float time;
//	uint flags;
//} constants;

const vec3 CUBE_COORDS[8] = 
{
		vec3( 50,  50,  50)
	,	vec3( 50,  50, -50)
	,	vec3( 50, -50,  50)
	,	vec3( 50, -50, -50)
	,	vec3(-50,  50,  50)
	,	vec3(-50,  50, -50)
	,	vec3(-50, -50,  50)
	,	vec3(-50, -50, -50)
};

void calculateIrrad(vec3 direction, out vec3 outDiffuseIrrad)
{
	vec3 diffuseIrrad = vec3(0.0);

	vec3 bitan1 = cross(direction, vec3(1, 0, 0));
	vec3 bitan2 = cross(direction, vec3(0, 0, 1));
	vec3 bitan = normalize((length(bitan1) > length(bitan2))? bitan1 : bitan2);
	vec3 tangent = normalize(cross(bitan, direction));

	int nSamples = 0;
	// In radians
	const float sampleDelta = 0.025;

	for(float phi = 0.0; phi < 2.0 * M_PI; phi += sampleDelta)
	{
		for(float theta = 0.0; theta < 0.5 * M_PI; theta += sampleDelta)
		{
			vec3 sampleDir = sphericalToCartesian(phi, theta);
			sampleDir = (sampleDir.x * tangent + sampleDir.y * bitan + sampleDir.z * direction);

			diffuseIrrad += texture(envMap, ENGINE_WORLD_TO_CUBE_DIR(sampleDir)).xyz * sin(theta) * cos(theta);

			nSamples++;
		}
	}

	diffuseIrrad *= M_PI / float(nSamples);
	outDiffuseIrrad = diffuseIrrad;
//	outDiffuseIrrad = vec3(length(bitan1), length(bitan2), length(tangent));
}

// We are rendering like usual world view but we are assigning x to z, y to x and z to y as in vulkan cube map z is fwd x is right and y is up
// While sampling we do the same 
// And also when rendering cubemap same has to assigned to each layers
void mainComp()
{
	ivec2 outSize = imageSize(outDiffuseIrradiance);
	vec2 outUV = vec2(gl_GlobalInvocationID.xy) / vec2(outSize);
	

	vec3 diffuseIrrad;
// All X's in Z faces 4, 5
// Face along +x
	vec3 direction = normalize(vec3(CUBE_COORDS[2].x, mix(CUBE_COORDS[2].yz, CUBE_COORDS[1].yz, outUV)));
	calculateIrrad(direction, diffuseIrrad);
	imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 4), vec4(diffuseIrrad, 1.0));
	
// Face along -x
	direction = normalize(vec3(CUBE_COORDS[4].x, mix(CUBE_COORDS[4].yz, CUBE_COORDS[7].yz, outUV)));
	calculateIrrad(direction, diffuseIrrad);
	imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 5), vec4(diffuseIrrad, 1.0));
	
// All Y's in X faces 0,1
// Face along +y
	vec2 faceSpaceUV = mix(CUBE_COORDS[0].xz, CUBE_COORDS[5].xz, outUV);
	direction = normalize(vec3(faceSpaceUV.x, CUBE_COORDS[0].y, faceSpaceUV.y));
	calculateIrrad(direction, diffuseIrrad);
	imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 0), vec4(diffuseIrrad, 1.0));
	
// Face along -y
	faceSpaceUV = mix(CUBE_COORDS[6].xz, CUBE_COORDS[3].xz, outUV);
	direction = normalize(vec3(faceSpaceUV.x, CUBE_COORDS[6].y, faceSpaceUV.y));
	calculateIrrad(direction, diffuseIrrad);
	imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 1), vec4(diffuseIrrad, 1.0));
		
// All Z's in Y faces 2, 3
// Face along +z, flipped uv because U goes from - Y to + Y and V goes from -X to +X
	direction = normalize(vec3(mix(CUBE_COORDS[6].xy, CUBE_COORDS[0].xy, outUV.yx), CUBE_COORDS[6].z));
	calculateIrrad(direction, diffuseIrrad);
	imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 2), vec4(diffuseIrrad, 1.0));
	
// Face along -z, flipped uv because U goes from - Y to + Y and V goes from +X to -X
	direction = normalize(vec3(mix(CUBE_COORDS[3].xy, CUBE_COORDS[5].xy, outUV.yx), CUBE_COORDS[3].z));
	calculateIrrad(direction, diffuseIrrad);
	imageStore(outDiffuseIrradiance, ivec3(gl_GlobalInvocationID.xy, 3), vec4(diffuseIrrad, 1.0));
}