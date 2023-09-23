/*!
 * \file Draw3DColoredPerInstance.vert.glsl
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

//
// Draws 3D geometries colored per instance and model matrix per instance
//
#define INSTANCED_SIMPLE3D_COLOR 1
#include "../../Common/VertexInputs.inl.glsl"
#undef INSTANCED_SIMPLE3D_COLOR

#include "../../Common/ViewDescriptors.inl.glsl"

layout(location=0) out vec4 outColor;

void mainVS()
{
    mat4 model;
    model[0] = vec4(x, 0);
    model[1] = vec4(y, 0);
    model[2] = vec4(cross(x, y), 0);
    model[3] = vec4(translation, 1);
    vec4 worldPos = model * vec4(position, 1);
    gl_Position = viewData.projection * viewData.invView * worldPos;
    outColor = color;
}