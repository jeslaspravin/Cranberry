#include "StbWrapper.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"

#if _DEBUG
#define STBI_FAILURE_USERMSG
#elif _NDEBUG
#define STBI_NO_FAILURE_STRINGS
#endif
#define STBI_NO_STDIO
#define STBI_ASSERT(x) debugAssert(x);
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

uint8* STB::loadFromMemory(uint8 const* buffer, int32 bufferLen, int32* x, int32* y, int32* channelsInFile, int desiredChannels)
{
    return stbi_load_from_memory(buffer, bufferLen, x, y, channelsInFile, desiredChannels);
}


float* STB::loadFloatFromMemory(uint8 const* buffer, int32 bufferLen, int32* x, int32* y, int32* channelsInFile, int desiredChannels)
{
    return stbi_loadf_from_memory(buffer, bufferLen, x, y, channelsInFile, desiredChannels);
}

void STB::deallocStbBuffer(void* data)
{
    stbi_image_free(data);
}

void STB::setLoadVerticaltFlipped(bool bFlip)
{
    stbi_set_flip_vertically_on_load(bFlip);
}

const AChar* STB::lastFailure()
{
    return stbi_failure_reason();
}

#undef STBI_NO_STDIO
#undef STBI_NO_FAILURE_STRINGS
#undef STBI_FAILURE_USERMSG
#undef STB_IMAGE_IMPLEMENTATION
#undef STBI_ASSERT
