/*!
 * \file SingleColorDescriptors.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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