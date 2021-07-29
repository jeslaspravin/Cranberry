#pragma once
#include "../../Platform/PlatformTypes.h"

namespace ImageUtils
{
    // Calculate RGB histogram from Color data
    // outHistogramR/G/B[binCount]
    void calcHistogramRGB(float* outHistogramR, float* outHistogramG, float* outHistogramB, uint32 binCount, const uint8* texels, uint32 sizeX, uint32 sizeY, uint32 channelNum);
}