/*!
 * \file DefaultStaticMeshPointLightDepth.frag.glsl
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

#include "../Common/CommonDefines.inl.glsl"

layout(set = SHADER_VARIANT_UNIQ_SET, binding = 0) uniform PointShadowDepthViews
{
    layout(offset = 384) vec4 lightPosFarPlane;
} lightViews;

layout(location = 0) in vec3 inWorldPos;

void mainFS()
{
    vec3 l2w = inWorldPos - lightViews.lightPosFarPlane.xyz;
    gl_FragDepth = (1.0 - (length(l2w) / lightViews.lightPosFarPlane.w));
}