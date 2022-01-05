/*!
 * \file VertexInstanceDescriptors.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#ifndef VERTEXINSTANCEDESCRIPTORS_INCLUDE
#define VERTEXINSTANCEDESCRIPTORS_INCLUDE

#include "CommonDefines.inl.glsl"

struct InstanceData
{
    mat4 model;
    mat4 invModel;
    // Index to shader unique param index
    uint shaderUniqIdx;
};

layout(set = INSTANCE_UNIQ_SET, binding = 0) readonly buffer Instances
{
    InstanceData instances[];
} instancesWrapper;

#endif // VERTEXINSTANCEDESCRIPTORS_INCLUDE