/*!
 * \file TexturedShader.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Vector4.h"

struct TexturedMeshData
{
    Vector4 meshColor;
    Vector4 rm_uvScale;
    uint32 diffuseMapIdx;
    uint32 normalMapIdx;
    uint32 armMapIdx;
};
