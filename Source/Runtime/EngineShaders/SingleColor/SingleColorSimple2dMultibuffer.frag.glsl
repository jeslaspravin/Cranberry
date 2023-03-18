/*!
 * \file SingleColorSimple2dMultibuffer.frag.glsl
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

#define SIMPLE2D 1
#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "SingleColorStageIO.inl.glsl"
#undef INPUT

#undef MULTIBUFFER
#undef SIMPLE2D

#include "SingleColorDescriptors.inl.glsl"

void mainFS()
{
    colorAttachment0 = materials.meshData[inMaterialIdx].meshColor;
    colorAttachment1 = vec4(0,0,1,1);
    colorAttachment2 = vec4(0, 0, 0, 1);
}