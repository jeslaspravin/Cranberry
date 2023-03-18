/*!
 * \file SingleColorStaticMeshMultibuffer.vert.glsl
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

#define OUTPUT 1
#include "SingleColorStageIO.inl.glsl"
#undef OUTPUT

#undef STATIC_MESH

void mainVS()
{
    InstanceData instance = instancesWrapper.instances[gl_InstanceIndex];
    vec4 worldPos = instance.model * vec4(position.xyz, 1);
    gl_Position = viewData.w2clip * worldPos;
    outWorldPosition = worldPos.xyz;
    outMaterialIdx = instance.shaderUniqIdx;
    outWorldNormal = (transpose(instance.invModel) * vec4(normal.xyz, 0)).xyz;
}