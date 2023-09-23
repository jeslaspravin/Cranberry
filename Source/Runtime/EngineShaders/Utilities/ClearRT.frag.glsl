/*!
 * \file ClearRT.frag.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#version 450

layout(location = 0) in vec2 inTextureCoord;

layout(location = 0) out vec4 colorAttachment0;

layout(set = 0, binding = 0) uniform ClearInfo
{
    vec4 clearColor;
} clearInfo;

void mainFS()
{
    colorAttachment0 = clearInfo.clearColor;
}