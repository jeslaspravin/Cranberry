/*!
 * \file TexturedDescriptors.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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