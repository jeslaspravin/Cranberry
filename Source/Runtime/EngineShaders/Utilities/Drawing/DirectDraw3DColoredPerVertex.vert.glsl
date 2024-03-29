/*!
 * \file DirectDraw3DColoredPerVertex.vert.glsl
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
// Draws 3D geometries colored per vertex, all vertices are in world space
//
#define SIMPLE3D_COLOR 1
#include "../../Common/VertexInputs.inl.glsl"
#undef SIMPLE3D_COLOR

#include "../../Common/ViewDescriptors.inl.glsl"

layout(location=0) out vec4 outColor;

layout(push_constant) uniform Constants
{
    float ptSize;
} constants;

void mainVS()
{
    gl_Position = viewData.projection * viewData.invView * vec4(position, 1);
    gl_PointSize = constants.ptSize;
    outColor = color;
}