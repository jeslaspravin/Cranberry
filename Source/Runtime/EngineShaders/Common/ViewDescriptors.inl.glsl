/*!
 * \file ViewDescriptors.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#ifndef VIEWDESCRIPTORS_INCLUDE
#define VIEWDESCRIPTORS_INCLUDE

#include "CommonDefines.inl.glsl"

layout(set = VIEW_UNIQ_SET, binding = 0) uniform ViewData
{
    mat4 view;
    mat4 invView;
    mat4 projection;
    mat4 invProjection;
} viewData;

// TODO(Jeslas) : Can be simplified for orthographics camera
vec3 getWorldPosition(vec4 screenPos)
{
    vec4 world = viewData.invProjection * screenPos;
    world = world/world.w;
    world = viewData.view * world;
    return world.xyz;
}

vec3 getViewSpacePosition(vec4 screenPos)
{
    vec4 viewSpace = viewData.invProjection * screenPos;
    viewSpace = viewSpace/viewSpace.w;
    return viewSpace.xyz;
}

vec3 viewFwd()
{
    return viewData.view[2].xyz;
}

vec3 viewPos()
{
    return viewData.view[3].xyz;
}

#endif // VIEWDESCRIPTORS_INCLUDE