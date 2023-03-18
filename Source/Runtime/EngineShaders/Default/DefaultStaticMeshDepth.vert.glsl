/*!
 * \file DefaultStaticMeshDepth.vert.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#version 450
#extension GL_GOOGLE_include_directive:enable

#define STATIC_MESH 1
#include "../Common/VertexInputs.inl.glsl"
#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/VertexInstanceDescriptors.inl.glsl"



#undef STATIC_MESH

void mainVS()
{
    gl_Position = viewData.projection * viewData.invView 
        * instancesWrapper.instances[gl_InstanceIndex].model * vec4(position.xyz, 1);
}