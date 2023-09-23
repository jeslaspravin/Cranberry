/*!
 * \file DrawImGui.vert.glsl
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

#define UI 1
#include "../Common/VertexInputs.inl.glsl"
#undef UI

layout(location = 0) out vec2 outTextureCoord;
layout(location = 1) out vec4 outColor;

layout(set = 0, binding = 0) uniform UiTransform
{
    vec2 scale;
    vec2 translate;
} uiTransform;

void mainVS()
{
    gl_Position = vec4((position * uiTransform.scale) + uiTransform.translate, 0, 1);
    outColor = color;
    outTextureCoord = uv;
}