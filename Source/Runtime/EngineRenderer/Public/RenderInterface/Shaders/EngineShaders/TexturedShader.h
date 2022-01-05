/*!
 * \file TexturedShader.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Vector4D.h"

struct TexturedMeshData
{
    Vector4D meshColor;
    Vector4D rm_uvScale;
    uint32 diffuseMapIdx;
    uint32 normalMapIdx;
    uint32 armMapIdx;
};
