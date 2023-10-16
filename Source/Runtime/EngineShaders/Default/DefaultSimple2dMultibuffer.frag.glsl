/*!
 * \file DefaultSimple2dMultibuffer.frag.glsl
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

#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "DefaultStageIO.inl.glsl"
#undef INPUT

#undef MULTIBUFFER

void mainFS()
{
    colorAttachment0 = vec4(1, 1, 1, 1);
    colorAttachment1 = vec4(0, 0, 1, 1);
    colorAttachment2 = vec4(0, 0, 0, 1);
}