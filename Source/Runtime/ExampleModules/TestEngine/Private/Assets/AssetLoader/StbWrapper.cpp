/*!
 * \file StbWrapper.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/AssetLoader/StbWrapper.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#if DEV_BUILD
#define STBI_FAILURE_USERMSG
#elif RELEASE_BUILD
#define STBI_NO_FAILURE_STRINGS
#endif
#define STBI_NO_STDIO
#define STBI_ASSERT(x) debugAssert(x)
#define STB_IMAGE_IMPLEMENTATION

COMPILER_PRAGMA(COMPILER_PUSH_WARNING)
COMPILER_PRAGMA(COMPILER_DISABLE_WARNING(WARN_UNNEEDED_INTERNAL_FUNCTION))

#include <stb_image.h>

COMPILER_PRAGMA(COMPILER_POP_WARNING)

uint8 *STB::loadFromMemory(uint8 const *buffer, int32 bufferLen, int32 *x, int32 *y, int32 *channelsInFile, int desiredChannels)
{
    return stbi_load_from_memory(buffer, bufferLen, x, y, channelsInFile, desiredChannels);
}

float *STB::loadFloatFromMemory(uint8 const *buffer, int32 bufferLen, int32 *x, int32 *y, int32 *channelsInFile, int desiredChannels)
{
    return stbi_loadf_from_memory(buffer, bufferLen, x, y, channelsInFile, desiredChannels);
}

void STB::deallocStbBuffer(void *data) { stbi_image_free(data); }

void STB::setLoadVerticalFlipped(bool bFlip) { stbi_set_flip_vertically_on_load_thread(bFlip); }

const AChar *STB::lastFailure() { return stbi_failure_reason(); }

#undef STBI_NO_STDIO
#undef STBI_NO_FAILURE_STRINGS
#undef STBI_FAILURE_USERMSG
#undef STB_IMAGE_IMPLEMENTATION
#undef STBI_ASSERT
