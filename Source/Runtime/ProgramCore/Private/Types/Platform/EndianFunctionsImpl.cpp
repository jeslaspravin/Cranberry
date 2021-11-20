#include "Types/Platform/PlatformFunctions.h"

namespace GPlatformConfigs
{
    PlatformEndian PLATFORM_ENDIAN;
}

PlatformEndian::PlatformEndian()
{
    uint32 temp = 1;
    variable = *reinterpret_cast<uint8*>(&temp) == 1 ? EndianType::Little : EndianType::Big;
}

bool PlatformEndian::isBigEndian() const
{
    return variable == EndianType::Big;
}

bool PlatformEndian::isLittleEndian() const
{
    return variable == EndianType::Little;
}