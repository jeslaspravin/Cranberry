/*!
 * \file DirectDraw3DColoredPerVertex.frag.glsl
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

layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 colorAttachment0;

void mainFS()
{
    colorAttachment0 = inColor;
}