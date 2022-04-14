/*!
 * \file ImageUtils.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/CoreTypes.h"

namespace ImageUtils
{
// Calculate RGB histogram from Color data
// outHistogramR/G/B[binCount]
void calcHistogramRGB(float *outHistogramR, float *outHistogramG, float *outHistogramB, uint32 binCount,
    const uint8 *texels, uint32 sizeX, uint32 sizeY, uint32 channelNum);
} // namespace ImageUtils