/*!
 * \file DrawQuadFromInputAttachment.frag.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#version 450

layout(location = 0) in vec2 inTextureCoord;

layout(location = 0) out vec4 colorAttachment0;

layout(input_attachment_index = 0,set = 0, binding = 0) uniform subpassInput quadTexture;

void mainFS()
{
    colorAttachment0 = subpassLoad(quadTexture);
}