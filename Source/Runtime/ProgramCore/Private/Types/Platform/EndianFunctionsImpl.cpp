/*!
 * \file EndianFunctionsImpl.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Platform/PlatformFunctions.h"

namespace GPlatformConfigs
{
PlatformEndian PLATFORM_ENDIAN;
}

PlatformEndian::PlatformEndian()
{
    uint32 temp = 1;
    variable = *reinterpret_cast<uint8 *>(&temp) == 1 ? EndianType::Little : EndianType::Big;
}

bool PlatformEndian::isBigEndian() const { return variable == EndianType::Big; }

bool PlatformEndian::isLittleEndian() const { return variable == EndianType::Little; }