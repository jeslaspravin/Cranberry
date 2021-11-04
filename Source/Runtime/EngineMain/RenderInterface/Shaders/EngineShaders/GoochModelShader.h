#pragma once
#include "../../../Core/Math/Vector4D.h"


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