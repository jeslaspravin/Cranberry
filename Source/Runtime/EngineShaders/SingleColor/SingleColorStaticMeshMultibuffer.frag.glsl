/*!
 * \file SingleColorStaticMeshMultibuffer.frag.glsl
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

#define STATIC_MESH 1
#define MULTIBUFFER 1
#include "../Common/ShaderOutputs.inl.glsl"

#define INPUT 1
#include "SingleColorStageIO.inl.glsl"
#undef INPUT

#undef STATE_MESH
#undef MULTIBUFFER

#include "SingleColorDescriptors.inl.glsl"

void mainFS()
{    
    MeshData meshDat = materials.meshData[inMaterialIdx];
    colorAttachment0 = meshDat.meshColor;
    colorAttachment1 = vec4((normalize(inWorldNormal) * 0.5) + 0.5, 1);
    colorAttachment2 = vec4(1, meshDat.roughness, meshDat.metallic, 1);
}