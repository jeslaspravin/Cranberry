#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "TexturedStageIO.inl.glsl"
#undef INPUT

#undef STATE_MESH
#undef MULTIBUFFER

#include "TexturedDescriptors.inl.glsl"

void mainFS()
{   
    vec2 uv = inTextureCoord / clamp(meshData.rm_uvScale.zw, vec2(0.001), abs(meshData.rm_uvScale.zw));
    vec3 arm = texture(armMap, uv).xyz;
    vec3 normal = texture(normalMap, uv).xyz;
    normal = (normal - 0.5) * 2;
    vec3 inNorm = normalize(inWorldNormal);
    vec3 inTangent = normalize(inWorldTangent);
    mat3 tbn;
    tbn[0] = inTangent;
    tbn[1] = cross(inNorm, inTangent);
    tbn[2] = inNorm;
    normal = normalize(tbn * normal);
    normal = (normal * 0.5) + 0.5;
    //colorAttachment0 = vec4(fract(uv), 0 ,1);
    colorAttachment0 = meshData.meshColor * texture(diffuseMap, uv);
    colorAttachment1 = vec4(normal, 1);
    colorAttachment2 = vec4(arm.x, arm.y * meshData.rm_uvScale.x, arm.z * meshData.rm_uvScale.y, 1);
}