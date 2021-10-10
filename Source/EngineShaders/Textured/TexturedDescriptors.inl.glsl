#ifndef TEXTURED_DESCRIPTORS_INCLUDE
#define TEXTURED_DESCRIPTORS_INCLUDE

#include "../Common/CommonDefines.inl.glsl"

struct MeshData
{
    vec4 meshColor;
    vec4 rm_uvScale;
    uint diffuseMapIdx;
    uint normalMapIdx;
    uint armMapIdx;
};

layout(set = SHADER_UNIQ_SET, binding = 0) readonly buffer TexturedMaterials
{
    MeshData meshData[];
} materials;

#endif