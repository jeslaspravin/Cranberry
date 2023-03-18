/*!
 * \file DebugVisDepthTexture.frag.glsl
 *
 * \author Jeslas Pravin
 * \date September 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#extension GL_GOOGLE_include_directive:enable

#include "../Common/ViewDescriptors.inl.glsl"

layout(location = 0) in vec2 inTextureCoord;
layout(location = 1) in vec2 inNdcCoord;

layout(location = 0) out vec4 colorAttachment0;

layout(constant_id = 1) const float DEPTH_NORMALIZE_RANGE = 5000;

layout(set = 0, binding = 0) uniform sampler2D quadTexture;

void mainFS()
{
    float depth = texture(quadTexture, inTextureCoord).x;
    vec4 world = viewData.invProjection * vec4(inNdcCoord, depth, 1);
    world = world/world.w;
    colorAttachment0 = vec4(fract(world.z/DEPTH_NORMALIZE_RANGE));
}