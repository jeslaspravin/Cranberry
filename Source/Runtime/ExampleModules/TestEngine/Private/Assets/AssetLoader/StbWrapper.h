/*!
 * \file StbWrapper.h
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

namespace STB
{
uint8 *loadFromMemory(uint8 const *buffer, int32 bufferLen, int32 *x, int32 *y, int32 *channelsInFile,
    int desiredChannels);
float *loadFloatFromMemory(uint8 const *buffer, int32 bufferLen, int32 *x, int32 *y,
    int32 *channelsInFile, int desiredChannels);

void deallocStbBuffer(void *data);
void setLoadVerticalFlipped(bool bFlip);
const AChar *lastFailure();
} // namespace STB
