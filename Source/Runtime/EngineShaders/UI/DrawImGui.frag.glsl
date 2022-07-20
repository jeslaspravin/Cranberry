/*!
 * \file DrawImGui.frag.glsl
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
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 colorAttachment0;

layout(set = 1, binding = 0) uniform sampler2D textureAtlas;

void mainFS()
{
    colorAttachment0 = inColor * texture(textureAtlas, inTextureCoord);
}