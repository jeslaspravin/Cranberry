/*!
 * \file DirectDraw3DColoredPerInstance.frag.glsl
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

#include "../../Common/CommonDefines.inl.glsl"

layout(location = 0) out vec4 colorAttachment0;

layout(push_constant) uniform Constants
{
    uint color;
} constants;

void mainFS()
{
    colorAttachment0 = UINT_RGBA_2_COLOR(constants.color);
}