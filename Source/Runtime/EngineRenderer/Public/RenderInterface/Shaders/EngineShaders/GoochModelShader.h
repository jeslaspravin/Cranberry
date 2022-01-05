/*!
 * \file GoochModelShader.h
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


struct GoochModelLightCommon
{
    uint32 lightsCount;
    float invLightsCount;
};


struct GoochModelLightData
{
    Vector4D warmOffsetAndPosX;
    Vector4D coolOffsetAndPosY;
    Vector4D highlightColorAndPosZ;
    Vector4D lightColorAndRadius;
};

struct GoochModelLightArray
{
    GoochModelLightData lights[10];
    uint32 count;
};