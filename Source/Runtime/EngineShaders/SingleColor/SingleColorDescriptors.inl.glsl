#ifndef SINGLE_COLOR_DESCRIPTORS_INCLUDE
#define SINGLE_COLOR_DESCRIPTORS_INCLUDE

#include "../Common/CommonDefines.inl.glsl"

struct MeshData
{
    vec4 meshColor;
    float roughness;
    float metallic;
};
layout(set = SHADER_UNIQ_SET, binding = 0) readonly buffer SingleColorMaterials
{
    MeshData meshData[];
} materials;

#endif