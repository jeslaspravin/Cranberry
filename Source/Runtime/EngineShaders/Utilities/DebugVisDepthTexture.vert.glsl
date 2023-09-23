/*!
 * \file DebugVisDepthTexture.vert.glsl
 *
 * \author Jeslas Pravin
 * \date September 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#extension GL_GOOGLE_include_directive:enable

#define SIMPLE3D 1
#include "../Common/VertexInputs.inl.glsl"
#undef SIMPLE3D

layout(location = 0) out vec2 outTextureCoord;
layout(location = 1) out vec2 outNdcCoord;

void mainVS()
{
    gl_Position = vec4(position,1);
    outTextureCoord = (position.xy + 1) * 0.5f;    
    // Fliping y since Quad draw uses vulkan screen coord top left -1,-1 bottom right 1,1. But our view/projection y coordinate is from bottom(-1) to top(1)
    outNdcCoord = vec2(position.x, -position.y);
}