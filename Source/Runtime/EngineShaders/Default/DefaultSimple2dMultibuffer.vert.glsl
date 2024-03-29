/*!
 * \file DefaultSimple2dMultibuffer.vert.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#version 450
#extension GL_GOOGLE_include_directive:enable

#define SIMPLE2D 1
#include "../Common/VertexInputs.inl.glsl"
#include "../Common/ViewDescriptors.inl.glsl"
#include "../Common/VertexInstanceDescriptors.inl.glsl"

#define OUTPUT 1
#include "DefaultStageIO.inl.glsl"
#undef OUTPUT

#undef SIMPLE2D

void mainVS()
{
    vec4 worldPos = instancesWrapper.instances[gl_InstanceIndex].model * vec4(position.xy, 0, 1);
    gl_Position = viewData.projection * viewData.invView * worldPos;
}