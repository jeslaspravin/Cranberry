/*!
 * \file DefaultStaticMeshDirectionalLightDepth.geom.glsl
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

layout(triangles, invocations = 8) in;
layout(triangle_strip, max_vertices = 3) out; 

layout(set = SHADER_VARIANT_UNIQ_SET, binding = 0) uniform DirectionalShadowCascadeViews
{
    mat4 cascadeW2Clip[8];
    uint cascadeCount;
} lightViews;

void mainGeo()
{
    if (gl_InvocationID >= lightViews.cascadeCount)
    {
        return;
    }

    for(int i = 0; i < 3; ++i)
    {
        gl_Position = lightViews.cascadeW2Clip[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}