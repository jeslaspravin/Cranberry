#pragma once
#include "Types/CoreTypes.h"

namespace STB
{
    uint8* loadFromMemory(uint8 const* buffer, int32 bufferLen, int32* x, int32* y, int32* channelsInFile, int desiredChannels);
    float* loadFloatFromMemory(uint8 const* buffer, int32 bufferLen, int32* x, int32* y, int32* channelsInFile, int desiredChannels);

    void deallocStbBuffer(void* data);
    void setLoadVerticalFlipped(bool bFlip);
    const AChar* lastFailure();
}
