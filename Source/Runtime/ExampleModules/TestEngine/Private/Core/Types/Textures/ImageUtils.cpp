/*!
 * \file ImageUtils.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ImageUtils.h"
#include "Math/Math.h"
#include "Math/Vector3.h"

void ImageUtils::calcHistogramRGB(
    float *outHistogramR, float *outHistogramG, float *outHistogramB, uint32 binCount, const uint8 *texels, uint32 sizeX, uint32 sizeY,
    uint32 channelNum
)
{
    memset(outHistogramR, 0u, binCount * sizeof(float));
    memset(outHistogramG, 0u, binCount * sizeof(float));
    memset(outHistogramB, 0u, binCount * sizeof(float));

    const uint32 pixelsCount = sizeX * sizeY;
    const float perBinDelta = 256.0f / binCount;
    const float perTexelWeight = 1.0f / pixelsCount;

    for (uint32 i = 0; i < pixelsCount; ++i)
    {
        const uint32 pixelStart = i * channelNum;

        Vector3 binIndices
            = Math::floor(Vector3(texels[pixelStart] / perBinDelta, texels[pixelStart + 1] / perBinDelta, texels[pixelStart + 2] / perBinDelta)
            );
        outHistogramR[uint32(binIndices.x())] += perTexelWeight;
        outHistogramG[uint32(binIndices.y())] += perTexelWeight;
        outHistogramB[uint32(binIndices.z())] += perTexelWeight;
    }
}
